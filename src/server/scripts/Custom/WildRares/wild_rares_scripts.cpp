#include "WildRareRules.h"
#include "../Affixes/AffixMgr.h"
#include "../Heartseed/HeartseedMgr.h"
#include "../Reload/ReloadRegistry.h"
#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "CreatureHooks.h"
#include "CustomClock.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "Log.h"
#include "LootMgr.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "WorldSession.h"
#include <list>
#include <mutex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

// Wild rares (DESIGN.md, Rares and discovery): ordinary open-world mobs sometimes spawn as a lesser rare with one or
// two scaled-down affixes and more health. A player's death nearby wears it off, so a wild rare never punishes hard.
namespace
{
    constexpr uint32 SPELL_WILD_RARE = 318100;
    rbac::RBACPermissions const GmPermission = static_cast<rbac::RBACPermissions>(98001);

    struct Config
    {
        bool Enable = true;
        float ChancePct = 2.0f;
        uint32 ZoneCap = 6;
        int64 CooldownSeconds = 900;
        bool IncludeNeutral = true;
        std::vector<uint32_t> CountWeights = { 75, 25 };   // index 0 = one affix
        float Intensity = 0.55f;
        uint32 HealthMinPct = 50;
        uint32 HealthMaxPct = 100;
        float XpBase = 0.10f;
        float XpPerAffix = 0.05f;
        float XpMax = 0.20f;
        uint32 LootRolls = 1;
        float EssenceChancePct = 3.0f;
        float ResetRadius = 40.0f;
        bool BotDeathsCount = false;
        uint32 VignetteId = 4;
        std::string Emote = "%s crackles with wild power!";
    };
    Config config;

    struct Wild
    {
        std::vector<uint32_t> Affixes;
        uint32 HealthPct = 0;      // extra health applied, so a reset can take exactly that back
        uint32 MapId = 0;
        uint32 ZoneId = 0;
        uint64 SpawnId = 0;
        bool Alive = true;         // false once dead: the record stays until the kill is paid and it respawns
        bool Paid = false;
    };

    struct Stats
    {
        uint64 Rolled = 0, Turned = 0, Missed = 0, ZoneFull = 0, OnCooldown = 0, Forced = 0, Resets = 0, Kills = 0;
    };

    std::mutex lock;   // spawn hooks run on every map's thread
    std::unordered_map<ObjectGuid, Wild> wilds;
    WildRares::ZoneTally tally;
    WildRares::Cooldowns cooldowns;
    std::unordered_set<uint32> denylist;
    Stats stats;
    float chanceOverride = -1.0f;   // `.wildrare chance`, until restart

    std::vector<uint32_t> ParseWeights(std::string const& text)
    {
        std::vector<uint32_t> weights;
        std::stringstream in(text);
        std::string part;
        while (std::getline(in, part, ','))
        {
            try { weights.push_back(uint32_t(std::stoul(part))); }
            catch (...) { return {}; }
        }
        return weights;
    }

    void LoadConfig()
    {
        config.Enable = sConfigMgr->GetBoolDefault("WildRares.Enable", true);
        config.ChancePct = std::max(0.0f, sConfigMgr->GetFloatDefault("WildRares.ChancePct", 2.0f));
        config.ZoneCap = uint32(std::max(0, sConfigMgr->GetIntDefault("WildRares.ZoneCap", 6)));
        config.CooldownSeconds = std::max(0, sConfigMgr->GetIntDefault("WildRares.SpawnCooldownSeconds", 900));
        config.IncludeNeutral = sConfigMgr->GetBoolDefault("WildRares.IncludeNeutral", true);
        std::vector<uint32_t> weights = ParseWeights(sConfigMgr->GetStringDefault("WildRares.AffixCountWeights", "75,25"));
        config.CountWeights = weights.empty() ? std::vector<uint32_t>{ 75, 25 } : weights;
        config.Intensity = Affixes::ClampIntensity(sConfigMgr->GetFloatDefault("WildRares.AffixIntensity", 0.55f));
        config.HealthMinPct = uint32(std::max(0, sConfigMgr->GetIntDefault("WildRares.HealthBonusMinPct", 95)));
        config.HealthMaxPct = uint32(std::max(0, sConfigMgr->GetIntDefault("WildRares.HealthBonusMaxPct", 160)));
        config.XpBase = std::max(0.0f, sConfigMgr->GetFloatDefault("WildRares.Xp.BaseLevelFraction", 0.10f));
        config.XpPerAffix = std::max(0.0f, sConfigMgr->GetFloatDefault("WildRares.Xp.PerAffixLevelFraction", 0.05f));
        config.XpMax = std::max(0.0f, sConfigMgr->GetFloatDefault("WildRares.Xp.MaxLevelFraction", 0.20f));
        config.LootRolls = uint32(std::max(0, sConfigMgr->GetIntDefault("WildRares.ExtraLootRolls", 1)));
        config.EssenceChancePct = std::max(0.0f, sConfigMgr->GetFloatDefault("WildRares.EssenceChancePct", 3.0f));
        config.ResetRadius = std::max(0.0f, sConfigMgr->GetFloatDefault("WildRares.DeathReset.Radius", 40.0f));
        config.BotDeathsCount = sConfigMgr->GetBoolDefault("WildRares.DeathReset.BotsCount", false);
        config.VignetteId = uint32(std::max(0, sConfigMgr->GetIntDefault("WildRares.Vignette.Id", 4)));
        config.Emote = sConfigMgr->GetStringDefault("WildRares.SpawnEmote", "%s crackles with wild power!");
    }

    bool LoadDenylist(std::size_t* rows)
    {
        std::unordered_set<uint32> loaded;
        if (QueryResult result = WorldDatabase.Query("SELECT `entry` FROM `rare_wild_denylist`"))
            do
                loaded.insert(result->Fetch()[0].GetUInt32());
            while (result->NextRow());
        if (rows)
            *rows = loaded.size();
        std::lock_guard<std::mutex> guard(lock);
        denylist.swap(loaded);
        return true;
    }

    float Chance()
    {
        return chanceOverride >= 0.0f ? chanceOverride : config.ChancePct;
    }

    bool InCity(Creature const* c)
    {
        constexpr uint32 cityFlags = AREA_FLAG_CAPITAL | AREA_FLAG_SLAVE_CAPITAL | AREA_FLAG_SLAVE_CAPITAL2 | AREA_FLAG_CITY | AREA_FLAG_SANCTUARY;
        for (uint32 id : { c->GetZoneId(), c->GetAreaId() })
            if (AreaTableEntry const* area = sAreaTableStore.LookupEntry(id))
                if (area->Flags & cityFlags)
                    return true;
        return false;
    }

    WildRares::Facts Gather(Creature const* c)
    {
        CreatureTemplate const* info = c->GetCreatureTemplate();
        WildRares::Facts f;
        f.IsPlayerControlled = c->IsPet() || c->IsTotem() || c->IsSummon() || c->IsGuardian() || c->IsCharmedOwnedByPlayerOrPlayer()
            || !c->GetOwnerGUID().IsEmpty() || !c->GetCharmerGUID().IsEmpty();
        f.HasSpawnId = c->GetSpawnId() != 0;
        f.InInstance = c->GetMap()->Instanceable();
        f.InCity = InCity(c);
        f.HasNpcRole = c->GetUInt32Value(UNIT_FIELD_NPC_FLAGS) || c->GetUInt32Value(UNIT_FIELD_NPC_FLAGS + 1) || info->npcflag
            || info->npcflag2 || info->GossipMenuId;
        f.NormalRank = info->rank == CREATURE_ELITE_NORMAL;
        f.Critter = info->type == CREATURE_TYPE_CRITTER || info->type == CREATURE_TYPE_NON_COMBAT_PET || info->type == CREATURE_TYPE_TOTEM
            || (info->flags_extra & (CREATURE_FLAG_EXTRA_TRIGGER | CREATURE_FLAG_EXTRA_CIVILIAN));
        f.Attackable = !c->HasFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_IMMUNE_TO_PC);
        FactionTemplateEntry const* faction = c->GetFactionTemplateEntry();
        f.Hostile = faction && (faction->IsHostileToPlayers() || (config.IncludeNeutral && faction->IsNeutralToAll()));
        {
            std::lock_guard<std::mutex> guard(lock);
            f.Denied = denylist.count(c->GetEntry()) != 0;
        }
        f.Alive = c->IsAlive();
        return f;
    }

    bool IsWild(Creature const* c, Wild* out = nullptr)
    {
        std::lock_guard<std::mutex> guard(lock);
        auto itr = wilds.find(c->GetGUID());
        if (itr == wilds.end())
            return false;
        if (out)
            *out = itr->second;
        return true;
    }

    // Wild rares carry the marker while alive: reading the aura keeps the vignette lookup lock-free on map threads.
    uint32_t WildVignette(Creature const* c)
    {
        return config.VignetteId && c->IsAlive() && c->HasAura(SPELL_WILD_RARE) ? config.VignetteId : 0;
    }

    // Players who already see the creature get its star added or taken away now, not at their next visibility pass.
    void RefreshVignette(Creature* c)
    {
        std::list<Player*> players;
        c->GetPlayerListInGrid(players, c->GetVisibilityRange());
        for (Player* player : players)
        {
            player->GetVignetteMgr().OnWorldObjectDisappear(static_cast<Creature const*>(c));
            if (player->HaveAtClient(c))
                player->GetVignetteMgr().OnWorldObjectAppear(static_cast<Creature const*>(c));
        }
    }

    void ScaleHealth(Creature* c, uint32 pct, bool apply)
    {
        if (!pct)
            return;
        float mult = 1.0f + pct / 100.0f;
        float healthPct = c->GetHealthPct();
        uint32 maxHealth = apply ? uint32(c->GetMaxHealth() * mult) : uint32(c->GetMaxHealth() / mult + 0.5f);
        c->SetMaxHealth(std::max<uint32>(1, maxHealth));
        if (c->IsAlive())
            c->SetHealth(std::max<uint32>(1, uint32(c->GetMaxHealth() * healthPct / 100.0f)));
    }

    uint64_t Uniform(uint64_t n)
    {
        return n <= 1 ? 0 : urand(0, uint32(std::min<uint64_t>(n, 0xFFFFFFFFull) - 1));
    }

    // Turns a creature wild. `affixes` empty rolls them; `forced` marks a GM command (callers skip the roll, cap and
    // cooldown checks).
    void MakeWild(Creature* c, std::vector<uint32_t> affixes, bool forced)
    {
        if (affixes.empty())
        {
            uint32 count = Affixes::RollCount(config.CountWeights, Uniform) + 1;
            affixes = sAffixMgr->Roll(Affixes::HOST_WILD_RARE, count);
        }
        Wild wild;
        wild.Affixes = affixes;
        wild.HealthPct = WildRares::HealthBonusPct(config.HealthMinPct, config.HealthMaxPct, float(rand_norm()));
        wild.MapId = c->GetMapId();
        wild.ZoneId = c->GetZoneId();
        wild.SpawnId = c->GetSpawnId();

        // Health before affixes: Juggernaut multiplies on top and takes only its own share back.
        ScaleHealth(c, wild.HealthPct, true);
        c->AddAura(SPELL_WILD_RARE, c);
        sAffixMgr->Apply(c, affixes, config.Intensity, Affixes::HOST_WILD_RARE);
        {
            std::lock_guard<std::mutex> guard(lock);
            wilds[c->GetGUID()] = wild;
            tally.Add(wild.MapId, wild.ZoneId);
            if (wild.SpawnId)
                cooldowns.Start(wild.SpawnId, CustomClock::Now(), config.CooldownSeconds);
            ++stats.Turned;
            if (forced)
                ++stats.Forced;
        }
        if (!config.Emote.empty())
            c->TextEmote(config.Emote);
        RefreshVignette(c);
    }

    // Affixes and extra health wear off; the creature is ordinary again and pays nothing extra.
    void Unwild(Creature* c, char const* why)
    {
        Wild wild;
        {
            std::lock_guard<std::mutex> guard(lock);
            auto itr = wilds.find(c->GetGUID());
            if (itr == wilds.end())
                return;
            wild = itr->second;
            if (wild.Alive)
                tally.Remove(wild.MapId, wild.ZoneId);
            wilds.erase(itr);
            ++stats.Resets;
        }
        sAffixMgr->Clear(c);
        c->RemoveAurasDueToSpell(SPELL_WILD_RARE);
        if (c->IsAlive())
            ScaleHealth(c, wild.HealthPct, false);
        RefreshVignette(c);
        TC_LOG_DEBUG("scripts", "Wild rare %s (%u) wore off: %s", c->GetName().c_str(), c->GetEntry(), why);
    }

    void OnDied(Creature* c)
    {
        {
            std::lock_guard<std::mutex> guard(lock);
            auto itr = wilds.find(c->GetGUID());
            if (itr == wilds.end() || !itr->second.Alive)
                return;
            itr->second.Alive = false;
            tally.Remove(itr->second.MapId, itr->second.ZoneId);
        }
        // The core takes the star away once the death completes (Creature::setDeathState).
    }

    // The record of a wild rare that died or left: no live count to give back unless it was still alive.
    void Drop(Creature* c)
    {
        bool had = false;
        {
            std::lock_guard<std::mutex> guard(lock);
            auto itr = wilds.find(c->GetGUID());
            if (itr != wilds.end())
            {
                if (itr->second.Alive)
                    tally.Remove(itr->second.MapId, itr->second.ZoneId);
                wilds.erase(itr);
                had = true;
            }
        }
        if (had)
            sAffixMgr->Forget(c->GetGUID());
    }

    void TryRoll(Creature* c)
    {
        if (!config.Enable || WildRares::Judge(Gather(c)) != WildRares::Verdict::Eligible)
            return;
        WildRares::RollResult result;
        {
            std::lock_guard<std::mutex> guard(lock);
            int64 now = CustomClock::Now();
            ++stats.Rolled;
            if ((stats.Rolled & 0x3FF) == 0)
                cooldowns.Prune(now);
            result = WildRares::RollSpawn(Chance(), tally.Live(c->GetMapId(), c->GetZoneId()), config.ZoneCap,
                cooldowns.Active(c->GetSpawnId(), now), float(rand_chance()));
            switch (result)
            {
                case WildRares::RollResult::Missed:   ++stats.Missed; break;
                case WildRares::RollResult::ZoneFull: ++stats.ZoneFull; break;
                case WildRares::RollResult::Cooldown: ++stats.OnCooldown; break;
                default: break;
            }
        }
        if (result == WildRares::RollResult::Wild)
            MakeWild(c, {}, false);
    }

    void OnCreatureEvent(Creature* c, CreatureHooks::Event event)
    {
        switch (event)
        {
            case CreatureHooks::Event::Spawned:
                TryRoll(c);
                break;
            case CreatureHooks::Event::Respawned:
                Drop(c);   // last life's record; its auras went with its death
                TryRoll(c);
                break;
            case CreatureHooks::Event::Evaded:
            {
                Wild wild;
                if (!IsWild(c, &wild) || !wild.Alive)
                    return;
                // An evade strips every aura; the extra health is not an aura and stays.
                if (!c->HasAura(SPELL_WILD_RARE))
                    c->AddAura(SPELL_WILD_RARE, c);
                sAffixMgr->Reapply(c);
                break;
            }
            case CreatureHooks::Event::Removed:
                Drop(c);
                break;
        }
    }

    bool CountsForReset(Player* player)
    {
        return config.BotDeathsCount || (player->GetSession() && !player->GetSession()->IsBot());
    }

    // A player died: every live wild rare that was fighting them, killed them, or stood close by wears off.
    void ResetNear(Player* victim, Unit* killer)
    {
        if (!config.Enable || !CountsForReset(victim))
            return;
        std::vector<ObjectGuid> candidates;
        {
            std::lock_guard<std::mutex> guard(lock);
            for (auto const& [guid, wild] : wilds)
                if (wild.Alive && wild.MapId == victim->GetMapId())
                    candidates.push_back(guid);
        }
        for (ObjectGuid guid : candidates)
        {
            Creature* c = ObjectAccessor::GetCreature(*victim, guid);
            if (!c || !c->IsAlive())
                continue;
            bool involved = c == killer || c->IsInCombatWith(victim) || victim->IsWithinDistInMap(c, config.ResetRadius);
            if (!involved)
                continue;
            Unwild(c, "a player died near it");
            ChatHandler(victim->GetSession()).PSendSysMessage("The wild power of |cffff8000%s|r fades.", c->GetName().c_str());
        }
    }

    void PayKill(Player* killer, Creature* killed)
    {
        Wild wild;
        {
            std::lock_guard<std::mutex> guard(lock);
            auto itr = wilds.find(killed->GetGUID());
            if (itr == wilds.end() || itr->second.Paid)
                return;
            itr->second.Paid = true;
            wild = itr->second;
            ++stats.Kills;
        }
        float fraction = WildRares::XpFraction(config.XpBase, config.XpPerAffix, uint32(wild.Affixes.size()), config.XpMax);

        std::vector<Player*> earners{ killer };
        if (Group* group = killer->GetGroup())
            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
                if (Player* member = ref->GetSource())
                    if (member != killer && member->IsAlive() && member->IsAtGroupRewardDistance(killed))
                        earners.push_back(member);
        for (Player* player : earners)
            if (uint32 xp = uint32(fraction * player->GetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP)))
            {
                uint32 level = player->GetLevel(), before = player->GetUInt32Value(PLAYER_FIELD_XP);
                player->GiveXP(xp, nullptr);
                if (player->GetLevel() == level && player->GetUInt32Value(PLAYER_FIELD_XP) == before)
                    continue;   // XP turned off or max level
                ChatHandler(player->GetSession()).PSendSysMessage("Wild rare bonus: %u XP%s.", xp,
                    player == killer && config.LootRolls ? Trinity::StringFormat(", %u extra loot roll%s", config.LootRolls,
                        config.LootRolls == 1 ? "" : "s").c_str() : "");
            }

        uint32 lootId = killed->GetCreatureTemplate()->lootid;
        if (LootTemplate const* table = lootId ? LootTemplates_Creature.GetLootFor(lootId) : nullptr)
            for (uint32 i = 0; i < config.LootRolls; ++i)
                table->Process(killed->loot, true, 1 << killed->GetMap()->GetDifficulty(), 0, killer);

        if (config.EssenceChancePct > 0.0f && roll_chance_f(config.EssenceChancePct))
        {
            std::vector<Heartseed::EssenceInfo> essences = sHeartseedMgr->GetEssences();
            if (!essences.empty())
            {
                Heartseed::EssenceInfo const& essence = essences[urand(0, uint32(essences.size() - 1))];
                if (sHeartseedMgr->AddEssence(killer, essence.Id, 1) == Heartseed::EssenceResult::Ok)
                    ChatHandler(killer->GetSession()).PSendSysMessage("The wild rare leaves an essence behind: |cff00ff96%s|r.",
                        essence.Name.c_str());
            }
        }
    }
}

// 318100 - Wild Rare marker (clone of a periodic dummy aura). Shows players what it is; its removal by death frees
// the zone slot and the minimap star. The affixes ride their own marker auras.
class spell_wild_rare_marker : public AuraScript
{
    PrepareAuraScript(spell_wild_rare_marker);

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        if (GetTargetApplication()->GetRemoveMode() != AURA_REMOVE_BY_DEATH)
            return;
        if (Creature* me = GetTarget()->ToCreature())
            OnDied(me);
    }

    void Register() override
    {
        AfterEffectRemove += AuraEffectRemoveFn(spell_wild_rare_marker::HandleRemove, EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
    }
};

class wild_rares_playerscript : public PlayerScript
{
public:
    wild_rares_playerscript() : PlayerScript("wild_rares_playerscript") { }

    void OnCreatureKill(Player* killer, Creature* killed) override
    {
        PayKill(killer, killed);
    }

    void OnPlayerKilledByCreature(Creature* killer, Player* killed) override
    {
        ResetNear(killed, killer);
    }

    void OnPVPKill(Player* killer, Player* killed) override
    {
        ResetNear(killed, killer);
    }
};

class wild_rares_worldscript : public WorldScript
{
public:
    wild_rares_worldscript() : WorldScript("wild_rares_worldscript") { }

    void OnConfigLoad(bool /*reload*/) override { LoadConfig(); }
};

class wild_rares_commandscript : public CommandScript
{
public:
    wild_rares_commandscript() : CommandScript("wild_rares_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> wildTable =
        {
            { "spawn",   &HandleSpawn,   GmPermission, Trinity::ChatCommands::Console::No },
            { "explain", &HandleExplain, GmPermission, Trinity::ChatCommands::Console::No },
            { "reset",   &HandleReset,   GmPermission, Trinity::ChatCommands::Console::No },
            { "stats",   &HandleStats,   GmPermission, Trinity::ChatCommands::Console::Yes },
            { "chance",  &HandleChance,  GmPermission, Trinity::ChatCommands::Console::Yes },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "wildrare", wildTable, GmPermission, Trinity::ChatCommands::Console::Yes },
        };
        return commandTable;
    }

    // The selected creature, or the nearest wild rare within 50 yd.
    static Creature* Pick(ChatHandler* handler)
    {
        if (Creature* selected = handler->getSelectedCreature())
            return selected;
        Player* player = handler->GetSession()->GetPlayer();
        std::list<Creature*> creatures;
        Trinity::AnyUnitInObjectRangeCheck check(player, 50.0f);
        Trinity::CreatureListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(player, creatures, check);
        player->VisitNearbyGridObject(50.0f, searcher);
        Creature* best = nullptr;
        for (Creature* c : creatures)
            if (c->IsAlive() && IsWild(c) && (!best || player->GetDistance(c) < player->GetDistance(best)))
                best = c;
        return best;
    }

    static void Report(ChatHandler* handler, Creature* c)
    {
        Wild wild;
        bool isWild = IsWild(c, &wild);
        WildRares::Verdict verdict = WildRares::Judge(Gather(c));
        int64 cooldown = 0;
        uint32 live = 0;
        {
            std::lock_guard<std::mutex> guard(lock);
            cooldown = c->GetSpawnId() ? cooldowns.Remaining(c->GetSpawnId(), CustomClock::Now()) : 0;
            live = tally.Live(c->GetMapId(), c->GetZoneId());
        }
        if (isWild)
            handler->PSendSysMessage("Wild rare %s (entry %u, guid %s): affixes %s at %.0f%% intensity; +%u%% health; %s; health %u/%u",
                c->GetName().c_str(), c->GetEntry(), c->GetGUID().ToString().c_str(), sAffixMgr->Describe(wild.Affixes).c_str(),
                sAffixMgr->Intensity(c->GetGUID()) * 100.0f, wild.HealthPct, wild.Alive ? "alive" : "dead",
                c->GetHealth(), c->GetMaxHealth());
        else
            handler->PSendSysMessage("Not wild: %s (entry %u, guid %s), health %u/%u",
                c->GetName().c_str(), c->GetEntry(), c->GetGUID().ToString().c_str(), c->GetHealth(), c->GetMaxHealth());
        handler->PSendSysMessage("Eligible: %s. Zone %u: %u live of cap %u. Spawn point %u: cooldown %llds. Chance %.2f%%.",
            WildRares::Describe(verdict), c->GetZoneId(), live, config.ZoneCap, uint32(c->GetSpawnId()), (long long)cooldown, Chance());
    }

    static bool ParseAffixes(ChatHandler* handler, std::stringstream& in, std::vector<uint32_t>& out)
    {
        std::string name;
        while (in >> name)
        {
            Affixes::Template const* t = sAffixMgr->FindByName(name);
            if (!t)
            {
                handler->PSendSysMessage("Unknown affix '%s' (see .rare affixes)", name.c_str());
                return false;
            }
            out.push_back(t->Id);
        }
        return true;
    }

    // .wildrare spawn [entry] [affix ...]: turns the selected creature wild, or with an entry a temporary hostile
    // creature at you. Affixes by name, or rolled.
    static bool HandleSpawn(ChatHandler* handler, char const* args)
    {
        std::stringstream in(args ? args : "");
        uint32 entry = 0;
        std::streampos start = in.tellg();
        if (!(in >> entry))
        {
            in.clear();
            in.seekg(start);
            entry = 0;
        }
        std::vector<uint32_t> affixes;
        if (!ParseAffixes(handler, in, affixes))
            return false;

        Creature* c = nullptr;
        if (entry)
        {
            Player* player = handler->GetSession()->GetPlayer();
            TempSummon* summon = player->SummonCreature(entry, player->GetNearPosition(5.0f, 0.0f), TEMPSUMMON_CORPSE_TIMED_DESPAWN, 120000ms);
            if (!summon)
            {
                handler->PSendSysMessage("Could not spawn creature %u", entry);
                return false;
            }
            summon->SetFaction(14);   // hostile to everyone, so a test creature never sides with the GM
            c = summon;
        }
        else
            c = handler->getSelectedCreature();
        if (!c || !c->IsAlive())
        {
            handler->SendSysMessage("Select a living creature, or use .wildrare spawn <entry> [affix ...]");
            return false;
        }
        if (IsWild(c))
            Unwild(c, "re-rolled by a GM");
        MakeWild(c, affixes, true);
        Report(handler, c);
        return true;
    }

    static bool HandleExplain(ChatHandler* handler, char const* /*args*/)
    {
        if (Creature* c = Pick(handler))
        {
            Report(handler, c);
            return true;
        }
        handler->SendSysMessage("Select a creature (no wild rare within 50 yd).");
        return true;
    }

    // .wildrare reset: wears the selected or nearest wild rare off, as a player's death would.
    static bool HandleReset(ChatHandler* handler, char const* /*args*/)
    {
        Creature* c = Pick(handler);
        if (!c || !IsWild(c))
        {
            handler->SendSysMessage("No wild rare selected or within 50 yd.");
            return true;
        }
        Unwild(c, "reset by a GM");
        Report(handler, c);
        return true;
    }

    static bool HandleStats(ChatHandler* handler, char const* /*args*/)
    {
        std::lock_guard<std::mutex> guard(lock);
        handler->PSendSysMessage("Wild rares: %u live, chance %.2f%%%s, zone cap %u, cooldown %llds (%u spawn points cooling), "
            "intensity %.2f, health +%u-%u%%",
            tally.Total(), Chance(), chanceOverride >= 0.0f ? " (override)" : "", config.ZoneCap, (long long)config.CooldownSeconds,
            uint32(cooldowns.Size()), config.Intensity, config.HealthMinPct, config.HealthMaxPct);
        handler->PSendSysMessage("Since start: %llu rolls, %llu turned wild (%llu forced), %llu missed, %llu zone full, %llu on cooldown, "
            "%llu kills paid, %llu resets",
            (unsigned long long)stats.Rolled, (unsigned long long)stats.Turned, (unsigned long long)stats.Forced,
            (unsigned long long)stats.Missed, (unsigned long long)stats.ZoneFull, (unsigned long long)stats.OnCooldown,
            (unsigned long long)stats.Kills, (unsigned long long)stats.Resets);
        for (auto const& [key, count] : tally.All())
            handler->PSendSysMessage("  map %u zone %u: %u live", key.first, key.second, count);
        return true;
    }

    // .wildrare chance <pct> | reset: overrides WildRares.ChancePct until restart.
    static bool HandleChance(ChatHandler* handler, char const* args)
    {
        std::string arg = args ? args : "";
        if (arg.empty())
        {
            handler->PSendSysMessage("Wild rare chance %.2f%%%s (config %.2f%%)", Chance(), chanceOverride >= 0.0f ? " (override)" : "",
                config.ChancePct);
            return true;
        }
        if (arg == "reset")
            chanceOverride = -1.0f;
        else
        {
            char* end = nullptr;
            float pct = std::strtof(arg.c_str(), &end);
            if (end == arg.c_str() || pct < 0.0f || pct > 100.0f)
            {
                handler->SendSysMessage("Use .wildrare chance <0-100> or .wildrare chance reset");
                return false;
            }
            chanceOverride = pct;
        }
        handler->PSendSysMessage("Wild rare chance now %.2f%%%s", Chance(), chanceOverride >= 0.0f ? " (override)" : "");
        return true;
    }
};

void AddSC_wild_rares()
{
    LoadConfig();
    std::size_t rows = 0;
    LoadDenylist(&rows);
    TC_LOG_INFO("server.loading", ">> Loaded %u wild rare denylist entries", uint32(rows));
    CreatureHooks::Register(&OnCreatureEvent);
    CreatureHooks::RegisterVignetteResolver(&WildVignette);
    if (!CustomReload::Tables().Register("rare_wild_denylist", "Wild rares: creature entries that never turn wild", []
    {
        std::size_t rows = 0;
        bool success = LoadDenylist(&rows);
        return CustomReload::Result{ success, rows, "Denylist published; applies to the next spawn rolls" };
    }))
        TC_LOG_ERROR("server", "Wild rare denylist reload registration failed");
    new aura_script<spell_wild_rare_marker>("spell_wild_rare_marker");
    new wild_rares_playerscript();
    new wild_rares_worldscript();
    new wild_rares_commandscript();
}
