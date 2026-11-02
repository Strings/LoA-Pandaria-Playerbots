#include "TelemetryMgr.h"

#include "CellImpl.h"
#include "Config.h"
#include "Creature.h"
#include "GitRevision.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Item.h"
#include "Group.h"
#include "Log.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellHistory.h"
#include "SpellInfo.h"
#include "WorldSession.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>

using Telemetry::JsonArray;
using Telemetry::JsonObject;

namespace
{
    namespace fs = std::filesystem;

    constexpr uint64 GatherHoldMs = 5000;           // a gathering cast keeps the episode on gathering this long
    constexpr uint64 PullWindowMs = 2000;           // a hostile cast this soon before combat makes it a pull
    constexpr float ServiceRange = 8.0f;
    constexpr size_t MaxListEntries = 32;           // auras, cooldowns and level gaps per snapshot

    enum Context : uint8
    {
        CTX_WORLD,
        CTX_DUNGEON,
        CTX_RAID,
        CTX_BATTLEGROUND,
        CTX_ARENA,
        CTX_SCENARIO,
    };

    char const* const ContextNames[] = { "world", "dungeon", "raid", "bg", "arena", "scenario" };

    uint64 NowMs()
    {
        using namespace std::chrono;
        return uint64(duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count());
    }

    uint8 ContextOf(Map const* map)
    {
        if (!map)
            return CTX_WORLD;
        if (map->IsBattleArena())
            return CTX_ARENA;
        if (map->IsBattleground())
            return CTX_BATTLEGROUND;
        if (map->IsScenario())
            return CTX_SCENARIO;
        if (map->IsRaid())
            return CTX_RAID;
        if (map->IsDungeon())
            return CTX_DUNGEON;
        return CTX_WORLD;
    }

    char const* RankName(Creature const* creature)
    {
        if (creature->isWorldBoss() || creature->IsDungeonBoss())
            return "boss";
        switch (creature->GetCreatureTemplate()->rank)
        {
            case 1: return "elite";
            case 2: return "rare_elite";
            case 3: return "boss";
            case 4: return "rare";
            default: return "normal";
        }
    }

    // Who a unit is in stable IDs only: creature entry or player class, never a name.
    void Identify(JsonObject& out, Unit const* unit)
    {
        if (Player const* player = unit->ToPlayer())
            out.Add("kind", "player").Add("class", player->GetClass()).Add("bot", player->GetSession()->IsBot());
        else if (Creature const* creature = unit->ToCreature())
            out.Add("kind", "creature").Add("entry", creature->GetEntry());
        else
            out.Add("kind", "unit");
    }

    Spell const* CurrentCast(Unit const* unit)
    {
        if (Spell const* spell = unit->GetCurrentSpell(CURRENT_GENERIC_SPELL))
            return spell;
        return unit->GetCurrentSpell(CURRENT_CHANNELED_SPELL);
    }

    float SpellProgress(Spell const* spell)
    {
        int32 total = spell->GetSpellInfo()->IsChanneled() ? spell->GetSpellInfo()->GetMaxDuration() : spell->GetCastTime();
        return Telemetry::CastProgress(total, spell->GetCurrentCastTimer());
    }

    std::string DrJson(Unit* unit)
    {
        JsonObject dr;
        for (uint32 group = DIMINISHING_BANISH; group < DIMINISHING_LIMITONLY; ++group)
            if (DiminishingLevels level = unit->GetDiminishing(DiminishingGroup(group)); level != DIMINISHING_LEVEL_1)
                dr.Add(std::to_string(group), uint32(level));
        return dr.Str();
    }

    bool IsGatherSpell(SpellInfo const* info)
    {
        return info->HasEffect(SPELL_EFFECT_OPEN_LOCK) || info->HasEffect(SPELL_EFFECT_SKINNING);
    }

    bool AtService(Player* player)
    {
        Creature* npc = ObjectAccessor::GetCreature(*player, player->GetTarget());
        if (!npc || !npc->IsWithinDistInMap(player, ServiceRange))
            return false;
        return npc->HasNpcFlag(NPCFlags(UNIT_NPC_FLAG_VENDOR | UNIT_NPC_FLAG_TRAINER | UNIT_NPC_FLAG_REPAIR |
            UNIT_NPC_FLAG_FLIGHTMASTER | UNIT_NPC_FLAG_INNKEEPER | UNIT_NPC_FLAG_BANKER | UNIT_NPC_FLAG_AUCTIONEER |
            UNIT_NPC_FLAG_MAILBOX | UNIT_NPC_FLAG_TRANSMOGRIFIER));
    }

    fs::path ResolveDirectory(std::string const& configured)
    {
        char const* source = GitRevision::GetSourceDirectory();
        fs::path root = (source && *source) ? fs::path(source) : fs::current_path();
        if (configured.empty())
            return root / "telemetry";
        fs::path path(configured);
        return path.is_relative() ? root / path : path;
    }

    std::string DateFolder(uint64 ms)
    {
        std::time_t seconds = std::time_t(ms / 1000);
        std::tm utc{};
#ifdef _WIN32
        gmtime_s(&utc, &seconds);
#else
        gmtime_r(&seconds, &utc);
#endif
        char buf[16];
        std::strftime(buf, sizeof(buf), "%Y-%m-%d", &utc);
        return buf;
    }
}

TelemetryMgr* TelemetryMgr::instance()
{
    static TelemetryMgr mgr;
    return &mgr;
}

void TelemetryMgr::LoadConfig()
{
    _enabled = sConfigMgr->GetBoolDefault("Telemetry.Enable", false);
    _chat = sConfigMgr->GetBoolDefault("Telemetry.Chat", false);
    _moveIntervalMs = uint32(std::max(50, sConfigMgr->GetIntDefault("Telemetry.MovementSampleMs", 200)));
    _idleTimeoutMs = uint64(std::max(1, sConfigMgr->GetIntDefault("Telemetry.IdleTimeoutSeconds", 300))) * 1000;
    _idleEpisodeMs = uint64(std::max(1, sConfigMgr->GetIntDefault("Telemetry.IdleEpisodeSeconds", 30))) * 1000;
    _discardWindowMs = uint64(std::max(0, sConfigMgr->GetIntDefault("Telemetry.DiscardWindowMinutes", 15))) * 60000;
    _flushIntervalMs = uint32(std::max(1000, sConfigMgr->GetIntDefault("Telemetry.FlushIntervalMs", 5000)));
    _episodeHoldMs = uint64(std::max(0, sConfigMgr->GetIntDefault("Telemetry.EpisodeHoldMs", 3000)));
    _packRadius = std::max(0.0f, sConfigMgr->GetFloatDefault("Telemetry.PackRadius", 60.0f));
    _directory = ResolveDirectory(sConfigMgr->GetStringDefault("Telemetry.Directory", ""));

    _interruptSpells.clear();
    std::istringstream spells(sConfigMgr->GetStringDefault("Telemetry.InterruptSpells",
        "6552 96231 147362 1766 15487 47528 57994 2139 116705 106839 78675 19647 119910"));
    for (uint32 id = 0; spells >> id; )
        _interruptSpells.push_back(id);
}

bool TelemetryMgr::Wanted(Player* player) const
{
    if (!_enabled || !player || !player->GetSession())
        return false;
    // 500 bots run every hook; they stop here unless some bot is tracked.
    return !player->GetSession()->IsBot() || _trackedBots > 0;
}

TelemetryMgr::Session* TelemetryMgr::Find(Player* player)
{
    auto itr = _sessions.find(player->GetGUID().GetCounter());
    return itr == _sessions.end() ? nullptr : &itr->second;
}

JsonObject TelemetryMgr::Row(Session const& s, char const* type, uint64 now) const
{
    JsonObject row;
    row.Add("v", Telemetry::SchemaVersion).Add("t", type).Add("ts", now).Add("sid", s.Id)
        .Add("pid", s.Guid.GetCounter()).Add("actor", s.Bot ? "scripted_bot" : "human").Add("ep", s.Episode.Id());
    if (!s.Intent.empty())
        row.Add("intent", s.Intent);
    return row;
}

void TelemetryMgr::Push(Session& s, uint64 now, JsonObject const& row, bool always)
{
    if (s.Pause.Paused() && !always)
        return;
    s.Rows.Push(now, row.Str());
}

void TelemetryMgr::SetPause(Session& s, uint32 reason, bool on, uint64 now)
{
    if (!s.Pause.Set(reason, on))
        return;
    if (s.Pause.Paused())
    {
        s.PausedSinceMs = now;
        Push(s, now, Row(s, "pause", now).Add("reasons", Telemetry::PauseReasonNames(s.Pause.Reasons())), true);
    }
    else
    {
        Push(s, now, Row(s, "resume", now).Add("paused_ms", now - s.PausedSinceMs), true);
        s.Episode.Break();
    }
}

void TelemetryMgr::Input(Session& s, uint64 now)
{
    s.LastInputMs = now;
    if (s.Pause.OnInput())
    {
        Push(s, now, Row(s, "resume", now).Add("paused_ms", now - s.PausedSinceMs), true);
        s.Episode.Break();
    }
}

TelemetryMgr::Lines TelemetryMgr::StartSession(Player* player, bool bot)
{
    uint64 now = NowMs();
    Session& s = _sessions[player->GetGUID().GetCounter()];
    s = Session();
    s.Guid = player->GetGUID();
    s.AccountId = player->GetSession()->GetAccountId();
    s.Bot = bot;
    s.StartMs = now;
    s.LastInputMs = now;
    s.Id = std::to_string(s.Guid.GetCounter()) + "-" + std::to_string(now);
    s.File = _directory / DateFolder(now) / (s.Id + ".jsonl");
    s.Target = player->GetTarget();
    s.InCombat = player->IsInCombat();
    s.Mounted = player->IsMounted();
    s.Taxi = player->IsInFlight();
    s.Dead = !player->IsAlive();
    s.Ghost = player->HasFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_GHOST);

    // Written at once rather than buffered, so a discard can never drop who the session belongs to.
    std::string start = Row(s, "session_start", now).Add("aid", s.AccountId).Add("class", player->GetClass())
        .Add("race", player->GetRace()).Add("lvl", player->GetLevel()).Add("build", GitRevision::GetHash()).Str();
    ++s.RowsWritten;
    if (player->IsGameMaster())
        SetPause(s, Telemetry::PAUSE_GM_MODE, true, now);
    return { { s.File, { start } } };
}

void TelemetryMgr::OnLogin(Player* player)
{
    if (!_enabled || player->GetSession()->IsBot())
        return;
    Lines lines;
    {
        std::lock_guard<std::mutex> guard(_lock);
        lines = StartSession(player, false);
    }
    Write(lines);
}

void TelemetryMgr::OnLogout(Player* player)
{
    Lines lines;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _sessions.find(player->GetGUID().GetCounter());
        if (itr == _sessions.end())
            return;
        Session& s = itr->second;
        uint64 now = NowMs();
        if (s.Fight.Active())
            s.Fight.End(now);
        Push(s, now, Row(s, "session_end", now).Add("dur_ms", now - s.StartMs), true);
        lines.emplace_back(s.File, s.Rows.TakeAll());
        if (s.Bot)
            --_trackedBots;
        _sessions.erase(itr);
    }
    Write(lines);
}

void TelemetryMgr::OnWorldUpdate(uint32 diff)
{
    _flushTimer += diff;
    if (_flushTimer < _flushIntervalMs)
        return;
    _flushTimer = 0;

    Lines lines;
    {
        std::lock_guard<std::mutex> guard(_lock);
        uint64 now = NowMs();
        for (auto& [guid, s] : _sessions)
        {
            std::vector<std::string> done = s.Rows.TakeFinal(now, _discardWindowMs);
            if (done.empty())
                continue;
            s.RowsWritten += done.size();
            lines.emplace_back(s.File, std::move(done));
        }
    }
    Write(lines);
}

void TelemetryMgr::OnShutdown()
{
    Lines lines;
    {
        std::lock_guard<std::mutex> guard(_lock);
        uint64 now = NowMs();
        for (auto& [guid, s] : _sessions)
        {
            Push(s, now, Row(s, "session_end", now).Add("dur_ms", now - s.StartMs).Add("shutdown", true), true);
            lines.emplace_back(s.File, s.Rows.TakeAll());
        }
        _sessions.clear();
        _trackedBots = 0;
    }
    Write(lines);
}

void TelemetryMgr::Write(Lines const& lines)
{
    for (auto const& [file, rows] : lines)
    {
        if (rows.empty())
            continue;
        std::error_code error;
        fs::create_directories(file.parent_path(), error);
        std::ofstream out(file, std::ios::app | std::ios::binary);
        if (!out)
        {
            TC_LOG_ERROR("server.loading", "Telemetry: cannot write %s", file.string().c_str());
            continue;
        }
        for (std::string const& row : rows)
            out << row << '\n';
    }
}

void TelemetryMgr::OnPlayerUpdate(Player* player)
{
    if (!Wanted(player))
        return;
    std::lock_guard<std::mutex> guard(_lock);
    Session* found = Find(player);
    if (!found)
        return;
    Session& s = *found;
    uint64 now = NowMs();

    SetPause(s, Telemetry::PAUSE_GM_MODE, player->IsGameMaster(), now);
    SetPause(s, Telemetry::PAUSE_AFK, player->isAFK(), now);
    // First, so every row this tick carries the episode it belongs to.
    UpdateEpisode(s, player, now);

    // Movement. Flight paths are not the player's own movement, so they log only as taxi edges.
    bool taxi = player->IsInFlight();
    if (!taxi)
    {
        Telemetry::MoveSample move{ player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(),
            player->GetOrientation(), player->GetUnitMovementFlags(), player->GetExtraUnitMovementFlags() };
        if (!s.HasMove || Telemetry::MoveSampleDue(s.LastMove, s.LastMoveMs, move, now, _moveIntervalMs))
        {
            bool moved = s.HasMove && Telemetry::MoveChanged(s.LastMove, move);
            if (moved && std::fabs(Telemetry::NormalizeAngle(move.O - s.LastMove.O)) > 0.05f)
                s.LastFacingChangeMs = now;
            if (moved)
                Input(s, now);
            if (!s.HasMove || moved)
                Push(s, now, Row(s, "move", now).Add("map", player->GetMapId()).Add("x", move.X, 2).Add("y", move.Y, 2)
                    .Add("z", move.Z, 2).Add("o", move.O, 3).Add("f", move.Flags).Add("f2", move.Flags2)
                    .Add("spd", player->GetSpeed(player->HasUnitMovementFlag(MOVEMENTFLAG_FLYING) ? MOVE_FLIGHT : MOVE_RUN), 2));
            s.HasMove = true;
            s.LastMove = move;
            s.LastMoveMs = now;
        }
    }
    if (taxi != s.Taxi)
    {
        s.Taxi = taxi;
        if (taxi)
            Decision(s, player, now, Row(s, "taxi_start", now), nullptr);
        else
        {
            Push(s, now, Row(s, "taxi_end", now));
            s.HasMove = false;
        }
    }

    if (now - s.LastInputMs >= _idleTimeoutMs)
        SetPause(s, Telemetry::PAUSE_IDLE, true, now);

    ObjectGuid target = player->GetTarget();
    if (target != s.Target)
    {
        s.Target = target;
        Input(s, now);
        Decision(s, player, now, Row(s, "target", now), ObjectAccessor::GetUnit(*player, target));
    }

    bool inCombat = player->IsInCombat();
    if (inCombat && !s.InCombat)
    {
        s.Fight.Start(now);
        bool pull = s.Pull || now - s.LastHostileCastMs <= PullWindowMs;
        Decision(s, player, now, Row(s, "combat_start", now).Add("pull", pull),
            ObjectAccessor::GetUnit(*player, target));
    }
    else if (!inCombat && s.InCombat)
    {
        Telemetry::FightSummary fight = s.Fight.End(now);
        char const* result = !player->IsAlive() ? "died" : fight.Kills ? "killed" : "left";
        Push(s, now, Row(s, "fight_end", now).Add("dur_ms", fight.DurationMs).Add("taken", fight.DamageTaken)
            .Add("done", fight.DamageDone).Add("kills", fight.Kills).Add("result", result));
        s.Pull = false;
    }
    else if (!inCombat && s.Fight.Active())
        s.Fight.End(now);      // damage that never put the character in combat
    s.InCombat = inCombat;

    bool mounted = player->IsMounted();
    if (mounted != s.Mounted)
    {
        s.Mounted = mounted;
        Decision(s, player, now, Row(s, mounted ? "mount" : "dismount", now), nullptr);
    }

    bool dead = !player->IsAlive();
    bool ghost = player->HasFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_GHOST);
    if (dead && !s.Dead)
    {
        s.DeathMs = now;
        JsonObject row = Row(s, "death", now);
        if (!s.PendingKiller.empty())
            row.AddRaw("killer", s.PendingKiller);
        Push(s, now, row);
        s.PendingKiller.clear();

        Map* map = player->GetMap();
        if (Group* group = player->GetGroup(); group && map && map->IsDungeon())
        {
            uint32 members = 0;
            bool wipe = true;
            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
                if (Player* member = ref->GetSource(); member && member->GetMap() == map)
                {
                    ++members;
                    wipe = wipe && !member->IsAlive();
                }
            if (wipe && members > 1)
                Push(s, now, Row(s, "wipe", now).Add("grp", members));
        }
    }
    if (dead && ghost && !s.Ghost)
        Decision(s, player, now, Row(s, "release", now).Add("since_death_ms", now - s.DeathMs), nullptr);
    if (!dead && s.Dead)
        Push(s, now, Row(s, "resurrect", now).Add("ghost", s.Ghost).Add("dead_ms", now - s.DeathMs));
    s.Dead = dead;
    s.Ghost = ghost;
}

void TelemetryMgr::UpdateEpisode(Session& s, Player* player, uint64 now)
{
    Telemetry::ActivityInputs inputs;
    inputs.InCombat = player->IsInCombat();
    inputs.Gathering = s.LastGatherMs && now - s.LastGatherMs <= GatherHoldMs;
    inputs.AtService = AtService(player);
    inputs.Traveling = player->IsMounted() || player->IsInFlight();
    inputs.Idle = now - s.LastInputMs >= _idleEpisodeMs;
    uint8 context = ContextOf(player->GetMap());
    if (!s.Episode.Update(Telemetry::ClassifyActivity(inputs), context, now, _episodeHoldMs))
        return;
    Push(s, now, Row(s, "episode", now).Add("act", Telemetry::ActivityName(s.Episode.Current()))
        .Add("ctx", ContextNames[context]).Add("since", s.Episode.StartMs()));
}

void TelemetryMgr::Decision(Session& s, Player* player, uint64 now, JsonObject row, Unit* target)
{
    if (s.Pause.Paused())
        return;
    row.AddRaw("ctx", Context(s, player, target, now, true));
    Push(s, now, row);
}

std::string TelemetryMgr::Context(Session const& s, Player* player, Unit* target, uint64 now, bool decision)
{
    JsonObject ctx;
    ctx.AddRaw("self", SelfJson(s, player, now));
    if (target)
        ctx.AddRaw("tgt", TargetJson(player, target, decision));
    else
        ctx.AddNull("tgt");
    ctx.AddRaw("env", EnvJson(player, target));
    ctx.AddRaw("place", PlaceJson(s, player, now));
    return ctx.Str();
}

std::string TelemetryMgr::SelfJson(Session const& s, Player* player, uint64 now)
{
    JsonObject self;
    self.Add("class", player->GetClass()).Add("spec", player->GetTalentSpecialization()).Add("lvl", player->GetLevel())
        .Add("hp", player->GetHealthPct(), 1);

    JsonArray power;
    for (uint8 type = 0; type < MAX_POWERS; ++type)
        if (int32 max = player->GetMaxPower(Powers(type)); max > 0)
            power.AddRaw(JsonArray().Add(type).Add(100.0 * player->GetPower(Powers(type)) / max, 1).Str());
    self.AddRaw("pw", power.Str());

    JsonArray buffs, debuffs;
    for (auto const& [slot, app] : *player->GetVisibleAuras())
    {
        JsonArray& list = app->IsPositive() ? buffs : debuffs;
        if (list.Size() < MaxListEntries)
            list.Add(app->GetBase()->GetId());
    }
    self.AddRaw("buffs", buffs.Str()).AddRaw("debuffs", debuffs.Str());

    // Spells still cooling down with the time left; everything else known is ready.
    JsonArray cooldowns;
    for (auto const& [spellId, entry] : player->GetSpellHistory()->GetCooldowns())
    {
        if (cooldowns.Size() >= MaxListEntries)
            break;
        if (uint32 left = player->GetSpellHistory()->GetCooldownDelay(spellId, entry.ItemId))
            cooldowns.AddRaw(JsonArray().Add(spellId).Add(left).Str());
    }
    self.AddRaw("cd", cooldowns.Str());

    bool flying = player->HasUnitMovementFlag(MOVEMENTFLAG_FLYING);
    bool swimming = player->HasUnitMovementFlag(MOVEMENTFLAG_SWIMMING);
    self.Add("ilvl", double(player->GetAverageItemLevel()), 1).Add("mounted", player->IsMounted())
        .Add("swim", swimming).Add("fly", flying).Add("combat", player->IsInCombat())
        .Add("spd", player->GetSpeed(flying ? MOVE_FLIGHT : swimming ? MOVE_SWIM : MOVE_RUN), 2)
        .Add("attackers", uint32(player->getAttackers().size()));

    uint64 since = 0;
    if (player->IsInCombat())
        self.Add("since_combat_ms", 0);
    else if (s.Fight.SinceEnd(now, since))
        self.Add("since_combat_ms", since);
    else
        self.AddNull("since_combat_ms");
    if (s.LastFacingChangeMs)
        self.Add("since_turn_ms", now - s.LastFacingChangeMs);
    else
        self.AddNull("since_turn_ms");
    self.AddRaw("dr", DrJson(player));
    return self.Str();
}

int32 TelemetryMgr::InterruptCooldown(Player* player) const
{
    for (uint32 spellId : _interruptSpells)
        if (player->HasSpell(spellId))
            return int32(player->GetSpellHistory()->GetCooldownDelay(spellId));
    return -1;
}

std::string TelemetryMgr::TargetJson(Player* player, Unit* target, bool decision)
{
    JsonObject tgt;
    Identify(tgt, target);
    if (Creature const* creature = target->ToCreature())
        tgt.Add("rank", RankName(creature)).Add("ctype", creature->GetCreatureType());
    else if (Player const* other = target->ToPlayer())
        tgt.Add("spec", other->GetTalentSpecialization());

    tgt.Add("self", target == player)
        .Add("lvl_gap", int32(target->GetLevel()) - int32(player->GetLevel()))
        .Add("hp", target->GetHealthPct(), 1)
        .Add("dead", !target->IsAlive())
        .Add("hostile", player->IsValidAttackTarget(target))
        .Add("dist", player->GetDistance(target), 2)
        .Add("ang", Telemetry::RelativeAngle(player->GetPositionX(), player->GetPositionY(), player->GetOrientation(),
            target->GetPositionX(), target->GetPositionY()), 3)
        .Add("dz", target->GetPositionZ() - player->GetPositionZ(), 2);
    // A raycast: decision events only, never per movement sample.
    if (decision && target != player)
        tgt.Add("los", player->IsWithinLOSInMap(target));

    if (Spell const* cast = CurrentCast(target))
        tgt.Add("cast", cast->GetSpellInfo()->Id).Add("cast_pct", SpellProgress(cast), 2);
    else
        tgt.Add("cast", 0);

    Unit* victim = target->GetVictim();
    char const* victimKind = !victim ? "none" : victim == player ? "self"
        : (victim->ToPlayer() && player->IsInSameRaidWith(victim->ToPlayer())) ? "ally" : "other";
    tgt.Add("victim", victimKind);

    if (Player* other = target->ToPlayer(); other && other != player)
    {
        int32 interrupt = InterruptCooldown(other);
        if (interrupt >= 0)
            tgt.Add("int_cd", interrupt);
        else
            tgt.AddNull("int_cd");
    }
    tgt.AddRaw("dr", DrJson(target));
    return tgt.Str();
}

std::string TelemetryMgr::EnvJson(Player* player, Unit* target)
{
    std::list<Unit*> hostiles;
    Trinity::AnyUnfriendlyUnitInObjectRangeCheck check(player, player, 40.0f);
    Trinity::UnitListSearcher<Trinity::AnyUnfriendlyUnitInObjectRangeCheck> searcher(player, hostiles, check);
    player->VisitNearbyObject(40.0f, searcher);

    std::vector<std::pair<float, int32>> gaps;
    uint32 within5 = 0, within15 = 0, within40 = 0, low = 0;
    for (Unit* unit : hostiles)
    {
        if (!player->IsValidAttackTarget(unit))
            continue;
        float dist = player->GetDistance(unit);
        ++within40;
        within15 += dist <= 15.0f;
        within5 += dist <= 5.0f;
        low += unit->GetHealthPct() < 20.0f;
        gaps.emplace_back(dist, int32(unit->GetLevel()) - int32(player->GetLevel()));
    }
    std::sort(gaps.begin(), gaps.end());
    JsonArray gapList;
    for (auto const& [dist, gap] : gaps)
        if (gapList.Size() < MaxListEntries)
            gapList.Add(gap);

    std::list<Player*> nearby;
    player->GetPlayerListInGrid(nearby, 40.0f);
    uint32 friendlyHumans = 0, friendlyBots = 0;
    for (Player* other : nearby)
        if (other != player && player->IsFriendlyTo(other))
            ++(other->GetSession()->IsBot() ? friendlyBots : friendlyHumans);

    JsonObject env;
    env.Add("h5", within5).Add("h15", within15).Add("h40", within40).AddRaw("gaps", gapList.Str()).Add("low", low)
        .Add("fp", friendlyHumans).Add("fb", friendlyBots);

    Group* group = player->GetGroup();
    uint32 roles[3] = { 0, 0, 0 };   // tank, healer, damage
    std::vector<Telemetry::Point> pack;
    uint32 onTarget = 0;
    if (group)
    {
        for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
        {
            Player* member = ref->GetSource();
            if (!member)
                continue;
            switch (member->GetRoleForGroup())
            {
                case ROLES_TANK: ++roles[0]; break;
                case ROLES_HEALER: ++roles[1]; break;
                default: ++roles[2]; break;
            }
            if (member == player || member->GetMap() != player->GetMap() || !member->IsAlive()
                || !member->IsWithinDistInMap(player, _packRadius))
                continue;
            pack.push_back({ member->GetPositionX(), member->GetPositionY(), member->GetOrientation() });
            if (target && member->GetVictim() == target)
                ++onTarget;
        }
    }
    env.Add("grp", group ? group->GetMembersCount() : 1u)
        .AddRaw("roles", JsonArray().Add(roles[0]).Add(roles[1]).Add(roles[2]).Str());
    if (pack.empty())
        env.AddNull("pack");
    else
    {
        Telemetry::PackSummary summary = Telemetry::SummarizePack(
            { player->GetPositionX(), player->GetPositionY(), player->GetOrientation() }, pack);
        env.AddRaw("pack", JsonObject().Add("n", summary.Count).Add("dist", summary.Distance, 2)
            .Add("ang", summary.Angle, 3).Add("head", summary.Heading, 3).Str());
    }
    // Initiating a swap (0) versus joining focus fire.
    env.Add("on_tgt", onTarget);
    return env.Str();
}

std::string TelemetryMgr::PlaceJson(Session const& s, Player* player, uint64 now)
{
    JsonObject place;
    place.Add("map", player->GetMapId()).Add("zone", player->GetZoneId()).Add("area", player->GetAreaId())
        .Add("inst", ContextNames[ContextOf(player->GetMap())]).Add("indoors", !player->IsOutdoors())
        .Add("sess_min", double(now - s.StartMs) / 60000.0, 2);
    return place.Str();
}

void TelemetryMgr::OnCastStart(Player* player, Spell const* spell)
{
    if (!Wanted(player))
        return;
    std::lock_guard<std::mutex> guard(_lock);
    Session* s = Find(player);
    if (!s)
        return;
    uint64 now = NowMs();
    Input(*s, now);

    SpellInfo const* info = spell->GetSpellInfo();
    Unit* target = spell->m_targets.GetUnitTarget();
    if (target && target != player && player->IsValidAttackTarget(target))
        s->LastHostileCastMs = now;
    if (IsGatherSpell(info))
        s->LastGatherMs = now;
    if (!target)
        target = ObjectAccessor::GetUnit(*player, player->GetTarget());

    JsonObject row = Row(*s, "cast_start", now);
    row.Add("spell", info->Id).Add("item", spell->m_CastItem ? spell->m_CastItem->GetEntry() : 0u)
        .Add("cast_ms", spell->GetCastTime()).Add("channel", info->IsChanneled());
    // Gaze lead: how long ago the character last turned.
    if (s->LastFacingChangeMs && now - s->LastFacingChangeMs <= 5000)
        row.Add("gaze_ms", now - s->LastFacingChangeMs);
    else
        row.AddNull("gaze_ms");
    Decision(*s, player, now, row, target);
}

void TelemetryMgr::OnCastDone(Player* player, Spell const* spell)
{
    // Triggered spells (procs, auras applied by other spells) are not the player's choices.
    if (spell->IsTriggered() || !Wanted(player))
        return;
    std::lock_guard<std::mutex> guard(_lock);
    Session* s = Find(player);
    if (!s)
        return;
    uint64 now = NowMs();
    Push(*s, now, Row(*s, "cast", now).Add("spell", spell->GetSpellInfo()->Id));
}

void TelemetryMgr::OnCastCancel(Player* player, Spell const* spell)
{
    if (!Wanted(player))
        return;
    std::lock_guard<std::mutex> guard(_lock);
    Session* s = Find(player);
    if (!s)
        return;
    uint64 now = NowMs();
    Input(*s, now);

    Unit* target = spell->m_targets.GetUnitTarget();
    if (!target)
        target = ObjectAccessor::GetUnit(*player, player->GetTarget());
    JsonObject row = Row(*s, "cast_cancel", now);
    row.Add("spell", spell->GetSpellInfo()->Id).Add("pct", SpellProgress(spell), 2)
        .Add("channel", spell->GetSpellInfo()->IsChanneled());
    // A fake cast: cancelled while the target could have interrupted it.
    if (Player* other = target ? target->ToPlayer() : nullptr; other && other != player && InterruptCooldown(other) >= 0)
        row.Add("tgt_int_ready", InterruptCooldown(other) == 0);
    else
        row.AddNull("tgt_int_ready");
    Decision(*s, player, now, row, target);
}

void TelemetryMgr::OnCommand(Player* player, char const* text)
{
    if (!Wanted(player) || player->GetSession()->GetSecurity() <= SEC_PLAYER)
        return;
    while (*text == '.' || *text == '!')
        ++text;
    if (!strncmp(text, "telemetry", 9))
        return;
    std::lock_guard<std::mutex> guard(_lock);
    if (Session* s = Find(player))
        SetPause(*s, Telemetry::PAUSE_GM_COMMAND, true, NowMs());
}

void TelemetryMgr::OnLootOpen(Player* player, WorldObject const* source)
{
    if (!Wanted(player))
        return;
    std::lock_guard<std::mutex> guard(_lock);
    Session* s = Find(player);
    if (!s)
        return;
    uint64 now = NowMs();
    Input(*s, now);
    JsonObject row = Row(*s, "loot_open", now);
    row.Add("src", source->GetTypeId() == TYPEID_UNIT ? "creature" : source->GetTypeId() == TYPEID_GAMEOBJECT ? "object" : "other")
        .Add("entry", source->GetEntry()).Add("dist", player->GetDistance(source), 2);
    Decision(*s, player, now, row, nullptr);
}

void TelemetryMgr::OnLootRoll(Player* player, uint32 itemId, uint8 choice)
{
    if (!Wanted(player))
        return;
    std::lock_guard<std::mutex> guard(_lock);
    Session* s = Find(player);
    if (!s)
        return;
    uint64 now = NowMs();
    Input(*s, now);
    static char const* const choices[] = { "pass", "need", "greed", "disenchant" };
    Decision(*s, player, now, Row(*s, "loot_roll", now).Add("item", itemId)
        .Add("choice", choice < 4 ? choices[choice] : "other"), nullptr);
}

void TelemetryMgr::OnObjective(Map* map, char const* kind, uint32 id, uint32 value)
{
    if (!_enabled)
        return;
    std::lock_guard<std::mutex> guard(_lock);
    if (_sessions.empty())
        return;
    uint64 now = NowMs();
    for (auto itr = map->GetPlayers().begin(); itr != map->GetPlayers().end(); ++itr)
    {
        Player* player = itr->GetSource();
        if (!player)
            continue;
        if (Session* s = Find(player))
            Push(*s, now, Row(*s, "objective", now).Add("kind", kind).Add("id", id).Add("val", value)
                .Add("x", player->GetPositionX(), 2).Add("y", player->GetPositionY(), 2).Add("z", player->GetPositionZ(), 2));
    }
}

void TelemetryMgr::OnChat(Player* player, uint32 type, uint32 lang, std::string const& msg)
{
    if (type == CHAT_MSG_ADDON || !Wanted(player))
        return;
    std::lock_guard<std::mutex> guard(_lock);
    Session* s = Find(player);
    if (!s)
        return;
    uint64 now = NowMs();
    Input(*s, now);
    if (!_chat)
        return;
    Decision(*s, player, now, Row(*s, "chat", now).Add("type", type).Add("lang", lang)
        .Add("len", uint32(msg.size())).Add("msg", msg), ObjectAccessor::GetUnit(*player, player->GetTarget()));
}

void TelemetryMgr::OnEmote(Player* player, uint32 textEmote)
{
    if (!Wanted(player))
        return;
    std::lock_guard<std::mutex> guard(_lock);
    Session* s = Find(player);
    if (!s)
        return;
    uint64 now = NowMs();
    Input(*s, now);
    Decision(*s, player, now, Row(*s, "emote", now).Add("emote", textEmote),
        ObjectAccessor::GetUnit(*player, player->GetTarget()));
}

void TelemetryMgr::OnDamage(Unit* attacker, Unit* victim, uint32 damage)
{
    if (!_enabled || !damage)
        return;
    Player* dealer = attacker ? attacker->GetCharmerOrOwnerPlayerOrPlayerItself() : nullptr;
    Player* taker = victim ? victim->ToPlayer() : nullptr;
    bool dealerWanted = dealer && Wanted(dealer);
    bool takerWanted = taker && Wanted(taker);
    if (!dealerWanted && !takerWanted)
        return;
    std::lock_guard<std::mutex> guard(_lock);
    uint64 now = NowMs();
    if (dealerWanted)
        if (Session* s = Find(dealer))
        {
            // The opening hit lands before the combat flag is seen, so it starts the fight and marks a pull.
            if (!s->Fight.Active() && dealer != victim)
            {
                s->Fight.Start(now);
                s->Pull = true;
            }
            s->Fight.AddDone(damage);
        }
    if (takerWanted)
        if (Session* s = Find(taker))
        {
            if (attacker != victim)
                s->Fight.Start(now);
            s->Fight.AddTaken(damage);
        }
}

void TelemetryMgr::OnKill(Player* killer, Unit* victim)
{
    if (!Wanted(killer))
        return;
    std::lock_guard<std::mutex> guard(_lock);
    Session* s = Find(killer);
    if (!s)
        return;
    uint64 now = NowMs();
    s->Fight.AddKill();
    JsonObject row = Row(*s, "kill", now);
    Identify(row, victim);
    row.Add("lvl_gap", int32(victim->GetLevel()) - int32(killer->GetLevel()));
    Push(*s, now, row);
}

void TelemetryMgr::OnKilledBy(Unit* killer, Player* victim)
{
    if (!Wanted(victim))
        return;
    std::lock_guard<std::mutex> guard(_lock);
    if (Session* s = Find(victim))
    {
        JsonObject who;
        Identify(who, killer);
        who.Add("lvl_gap", int32(killer->GetLevel()) - int32(victim->GetLevel()));
        s->PendingKiller = who.Str();
    }
}

void TelemetryMgr::OnZone(Player* player, uint32 zone, uint32 area)
{
    if (!Wanted(player))
        return;
    std::lock_guard<std::mutex> guard(_lock);
    if (Session* s = Find(player))
    {
        uint64 now = NowMs();
        Push(*s, now, Row(*s, "zone", now).Add("map", player->GetMapId()).Add("zone", zone).Add("area", area));
    }
}

bool TelemetryMgr::Describe(Player* player, std::string& out)
{
    std::lock_guard<std::mutex> guard(_lock);
    Session* s = Find(player);
    if (!s)
        return false;
    std::ostringstream text;
    text << "session " << s->Id << ", " << (s->Pause.Paused() ? "paused (" + Telemetry::PauseReasonNames(s->Pause.Reasons()) + ")" : std::string("logging"))
         << ", episode " << s->Episode.Id() << " (" << Telemetry::ActivityName(s->Episode.Current()) << ", "
         << ContextNames[s->Episode.Context()] << ")";
    if (!s->Intent.empty())
        text << ", intent " << s->Intent;
    text << ", " << s->Rows.Size() << " rows buffered, " << s->RowsWritten << " written to " << s->File.filename().string();
    out = text.str();
    return true;
}

bool TelemetryMgr::SetManualPause(Player* player, bool paused)
{
    std::lock_guard<std::mutex> guard(_lock);
    Session* s = Find(player);
    if (!s)
        return false;
    uint64 now = NowMs();
    SetPause(*s, Telemetry::PAUSE_MANUAL, paused, now);
    if (!paused)
        Input(*s, now);     // resuming by hand also ends GM-command and idle pauses
    return true;
}

bool TelemetryMgr::Discard(Player* player, uint32 minutes, size_t& dropped, uint32& appliedMinutes)
{
    std::lock_guard<std::mutex> guard(_lock);
    Session* s = Find(player);
    if (!s)
        return false;
    uint64 now = NowMs();
    appliedMinutes = std::min(minutes, DiscardWindowMinutes());
    dropped = s->Rows.Discard(now, uint64(appliedMinutes) * 60000);
    Push(*s, now, Row(*s, "discard", now).Add("span_ms", uint64(appliedMinutes) * 60000).Add("rows", uint64(dropped)), true);
    s->Episode.Break();
    return true;
}

bool TelemetryMgr::Tag(Player* player, std::string const& intent)
{
    std::lock_guard<std::mutex> guard(_lock);
    Session* s = Find(player);
    if (!s)
        return false;
    uint64 now = NowMs();
    s->Intent = intent;
    Push(*s, now, Row(*s, "intent", now).Add("value", intent));
    return true;
}

bool TelemetryMgr::Snapshot(Player* player, std::string& out)
{
    std::lock_guard<std::mutex> guard(_lock);
    Session* s = Find(player);
    if (!s)
        return false;
    out = Context(*s, player, ObjectAccessor::GetUnit(*player, player->GetTarget()), NowMs(), true);
    return true;
}

bool TelemetryMgr::Flush(Player* player, size_t& written)
{
    Lines lines;
    {
        std::lock_guard<std::mutex> guard(_lock);
        Session* s = Find(player);
        if (!s)
            return false;
        std::vector<std::string> rows = s->Rows.TakeAll();
        written = rows.size();
        s->RowsWritten += rows.size();
        lines.emplace_back(s->File, std::move(rows));
    }
    Write(lines);
    return true;
}

bool TelemetryMgr::Track(Player* bot, bool on)
{
    if (!_enabled || !bot->GetSession()->IsBot())
        return false;
    Lines lines;
    bool stop = false;
    {
        std::lock_guard<std::mutex> guard(_lock);
        bool tracked = Find(bot) != nullptr;
        if (on && !tracked)
        {
            ++_trackedBots;
            lines = StartSession(bot, true);
        }
        stop = !on && tracked;
    }
    if (stop)
        OnLogout(bot);      // ends the session the way a logout does
    else
        Write(lines);
    return true;
}
