/*
 * Battleground queue fill. See BgQueueFill.h.
 */
#include "BgQueueFill.h"

#include "BattleGroundJoinAction.h"   // IsSupportedBgQueue
#include "Battleground.h"
#include "BattlegroundMgr.h"
#include "BattlegroundQueue.h"
#include "BotScenario.h"
#include "Config.h"
#include "DBCStores.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "PlayerbotAIConfig.h"
#include "Playerbots.h"
#include "RandomPlayerbotMgr.h"
#include "Timer.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <algorithm>
#include <ctime>
#include <map>
#include <random>
#include <set>
#include <sstream>
#include <vector>

namespace
{
    struct Config
    {
        bool enable = true;
        uint32 intervalMs = 1000;
        uint32 delaySeconds = 0;
        uint32 teamSize = 0;
        bool loginOffline = true;
        uint32 maxExtraLogins = 60;
        uint32 extraStaySeconds = 300;
        uint32 settleSeconds = 3;
        uint32 loginTimeoutSeconds = 60;
        bool fastAccept = true;
        uint32 queueTimeoutSeconds = 120;
    };

    // Read every pass, quietly (an older playerbots.conf without the keys must not log "Missing name" each second).
    Config ReadConfig()
    {
        Config c;
        c.enable = sConfigMgr->GetIntDefault("AiPlayerbot.BgFill.Enable", 1, true) != 0;
        c.intervalMs = uint32(std::max(100, sConfigMgr->GetIntDefault("AiPlayerbot.BgFill.CheckIntervalMs", 1000, true)));
        c.delaySeconds = uint32(std::max(0, sConfigMgr->GetIntDefault("AiPlayerbot.BgFill.DelaySeconds", 0, true)));
        c.teamSize = uint32(std::max(0, sConfigMgr->GetIntDefault("AiPlayerbot.BgFill.TeamSize", 0, true)));
        c.loginOffline = sConfigMgr->GetIntDefault("AiPlayerbot.BgFill.LoginOffline", 1, true) != 0;
        c.maxExtraLogins = uint32(std::max(0, sConfigMgr->GetIntDefault("AiPlayerbot.BgFill.MaxExtraLogins", 60, true)));
        c.extraStaySeconds = uint32(std::max(0, sConfigMgr->GetIntDefault("AiPlayerbot.BgFill.ExtraBotStaySeconds", 300, true)));
        c.settleSeconds = uint32(std::max(0, sConfigMgr->GetIntDefault("AiPlayerbot.BgFill.LoginSettleSeconds", 3, true)));
        c.loginTimeoutSeconds = uint32(std::max(5, sConfigMgr->GetIntDefault("AiPlayerbot.BgFill.LoginTimeoutSeconds", 60, true)));
        c.fastAccept = sConfigMgr->GetIntDefault("AiPlayerbot.BgFill.FastAccept", 1, true) != 0;
        c.queueTimeoutSeconds = uint32(std::max(0, sConfigMgr->GetIntDefault("AiPlayerbot.BgFill.QueueTimeoutSeconds", 120, true)));
        return c;
    }

    struct Key
    {
        uint32 queueTypeId;
        uint32 bracketId;
        bool operator<(Key const& o) const
        {
            return queueTypeId != o.queueTypeId ? queueTypeId < o.queueTypeId : bracketId < o.bracketId;
        }
    };

    // One (queue, bracket) a real player waits in.
    struct Demand
    {
        BattlegroundTypeId bgTypeId = BATTLEGROUND_TYPE_NONE;
        PvPDifficultyEntry const* bracket = nullptr;
        uint32 target = 0;
        uint32 minPerTeam = 0;
        uint32 waiting[2] = { 0, 0 };      // not-invited queue entries per TeamId, players and bots
        uint32 realPlayers = 0;
        uint32 oldestWaitMs = 0;
        uint32 queuedNow[2] = { 0, 0 };    // bots this pass queued
        uint32 loginsNow[2] = { 0, 0 };    // offline bots this pass started logging in
        uint32 pending[2] = { 0, 0 };      // extra bots for this key still logging in or settling
        uint32 shortfall[2] = { 0, 0 };    // still missing after this pass (no bot to queue or log in)
    };

    // An offline random bot this module logged in.
    struct Extra
    {
        Key key;
        uint8 team;            // TeamId
        time_t loginAt;
        time_t lastBusy;       // last pass it was in a battleground, a queue or a group
        bool queued = false;   // this module queued it at least once
        bool arrived = false;  // has been in world
    };

    uint32 sTimer = 0;
    bool sForce = false;
    std::map<ObjectGuid::LowType, Extra> sExtras;
    // A bot this module queued that has not entered yet.
    struct Queued
    {
        Key key;
        time_t lastDemand;     // last pass a real player still waited in its (queue, bracket)
    };
    std::map<ObjectGuid, Queued> sOurQueued;
    std::vector<std::string> sLastStatus;
    uint32 sTotalQueued = 0;
    uint32 sTotalLogins = 0;

    char const* TeamLetter(uint32 team) { return team == TEAM_ALLIANCE ? "A" : "H"; }

    bool IsRealPlayer(Player* player)
    {
        if (!player)
            return false;
        PlayerbotAI* botAI = GET_PLAYERBOT_AI(player);
        return !botAI || botAI->IsRealPlayer();
    }

    // Can the core take this bot into this (queue, bracket) right now, without pulling it out of anything that matters?
    bool CanQueue(Player* bot, Key const& key, Demand const& demand, Battleground* bgTemplate, Config const& cfg)
    {
        if (!bot->IsInWorld() || bot->IsBeingTeleported() || bot->IsInFlight() || bot->IsInCombat())
            return false;
        if (bot->InBattleground() || bot->InArena() || bot->InBattlegroundQueue() || bot->inRandomLfgDungeon() ||
            bot->IsUsingLfg(false))
            return false;
        if (bot->GetGroup())   // a party with a real player, a bot dungeon group or a battlefield raid: leave it alone
            return false;
        if (!sRandomPlayerbotMgr->IsRandomBot(bot))
            return false;
        if (BotScenario::BgJoinBlocked(bot) || BotScenario::WantsTrail(bot))
            return false;
        if (time(nullptr) - time_t(bot->GetInGameTime()) < time_t(cfg.settleSeconds))
            return false;

        PvPDifficultyEntry const* own = sDBCManager.GetBattlegroundBracketByLevel(bgTemplate->GetMapId(), bot->GetLevel());
        if (!own || uint32(own->GetBracketId()) != key.bracketId)
            return false;
        if (!bot->GetBGAccessByLevel(demand.bgTypeId) || !bot->HasFreeBattlegroundQueueId() ||
            !bot->CanJoinToBattleground(bgTemplate))
            return false;
        return true;
    }

    // The tail of BGJoinAction::JoinQueue (WorldSession::HandleBattlemasterJoinOpcode for one player).
    bool QueueBot(Player* bot, BattlegroundQueueTypeId queueTypeId, Demand const& demand, Battleground* bgTemplate, Key const& key)
    {
        BattlegroundQueue& bgQueue = sBattlegroundMgr->GetBattlegroundQueue(queueTypeId);
        GroupQueueInfo* ginfo = bgQueue.AddGroup(bot, nullptr, demand.bgTypeId, demand.bracket, 0, false, false, 0, 0);
        if (!ginfo)
            return false;

        uint32 const avgWaitTime = bgQueue.GetAverageQueueWaitTime(ginfo, demand.bracket->GetBracketId());
        uint32 const queueSlot = bot->AddBattlegroundQueueId(queueTypeId);

        WorldPacket data;
        sBattlegroundMgr->BuildBattlegroundStatusPacket(&data, bgTemplate, bot, queueSlot, STATUS_WAIT_QUEUE, avgWaitTime, 0, 0);
        bot->GetSession()->SendPacket(&data);

        sOurQueued[bot->GetGUID()] = Queued{ key, time(nullptr) };
        return true;
    }

    // Offline random bots of this bracket and faction, at most `count`, not already logging in.
    std::vector<std::pair<ObjectGuid::LowType, uint32>> FindOfflineBots(Demand const& demand, uint32 team, uint32 count)
    {
        std::vector<std::pair<ObjectGuid::LowType, uint32>> found;
        if (!count)
            return found;

        // Ten times what is needed, at random: most rows are filtered out by faction and account below.
        std::ostringstream sql;
        sql << "SELECT guid, account, race FROM characters WHERE online = 0 AND level BETWEEN "
            << uint32(demand.bracket->MinLevel) << " AND " << uint32(demand.bracket->MaxLevel)
            << " ORDER BY RAND() LIMIT " << (count * 10 + 20);
        QueryResult result = CharacterDatabase.Query(sql.str().c_str());
        if (!result)
            return found;

        uint32 const wantedTeam = team == TEAM_ALLIANCE ? uint32(ALLIANCE) : uint32(HORDE);
        do
        {
            Field* fields = result->Fetch();
            ObjectGuid::LowType const guid = fields[0].GetUInt32();
            uint32 const account = fields[1].GetUInt32();
            uint8 const race = fields[2].GetUInt8();
            if (Player::TeamForRace(race) != wantedTeam)
                continue;
            if (!sPlayerbotAIConfig->IsInRandomAccountList(account))
                continue;
            if (sExtras.count(guid) || ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(guid)))
                continue;
            found.emplace_back(guid, account);
        } while (found.size() < count && result->NextRow());

        return found;
    }

    void LoginExtra(ObjectGuid::LowType guid, Key const& key, uint32 team)
    {
        // The random-bot manager only keeps a bot it counts as current ("add"); hold its randomize and teleport for the
        // same time so it keeps the level and place it was picked for until this module releases it again.
        sRandomPlayerbotMgr->SetValue(guid, "add", 1);
        sRandomPlayerbotMgr->SetValue(guid, "randomize", 1);
        sRandomPlayerbotMgr->SetValue(guid, "teleport", 1);
        sRandomPlayerbotMgr->AddCurrentBotForTest(guid);
        sRandomPlayerbotMgr->AddPlayerBot(ObjectGuid::Create<HighGuid::Player>(guid), 0);

        time_t const now = time(nullptr);
        Extra extra;
        extra.key = key;
        extra.team = uint8(team);
        extra.loginAt = now;
        extra.lastBusy = now;
        sExtras[guid] = extra;
    }

    // Logged-in extras: track activity, drop failed logins, release idle ones; count the ones still on their way.
    void UpdateExtras(Config const& cfg, std::map<Key, Demand>& demands)
    {
        time_t const now = time(nullptr);
        for (auto itr = sExtras.begin(); itr != sExtras.end();)
        {
            Extra& extra = itr->second;
            Player* bot = ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(itr->first));
            bool const inWorld = bot && bot->IsInWorld();

            if (!inWorld)
            {
                // Between maps (a battleground port): keep it.
                if (bot && extra.arrived)
                {
                    ++itr;
                    continue;
                }

                // Logged out by someone else after arriving, or never arrived: forget it.
                if (extra.arrived || now - extra.loginAt > time_t(cfg.loginTimeoutSeconds))
                {
                    if (!extra.arrived)
                        sRandomPlayerbotMgr->SetValue(itr->first, "add", 0);
                    itr = sExtras.erase(itr);
                    continue;
                }

                auto demand = demands.find(extra.key);
                if (demand != demands.end())
                    ++demand->second.pending[extra.team];
                ++itr;
                continue;
            }

            extra.arrived = true;

            if (bot->InBattleground() || bot->InBattlegroundQueue() || bot->GetGroup())
                extra.lastBusy = now;

            if (!extra.queued && !bot->InBattlegroundQueue())
            {
                auto demand = demands.find(extra.key);
                if (demand != demands.end() && now - extra.loginAt <= time_t(cfg.loginTimeoutSeconds))
                    ++demand->second.pending[extra.team];
            }

            if (now - extra.lastBusy >= time_t(cfg.extraStaySeconds) && now - extra.loginAt >= time_t(cfg.settleSeconds))
            {
                // The manager logs it out on its next pass over the bot (RandomPlayerbotMgr::ProcessBot, "add" = 0).
                sRandomPlayerbotMgr->SetValue(itr->first, "add", 0);
                TC_LOG_DEBUG("playerbots", "[BGFILL] release extra bot %u %s:%u <%s>", itr->first, TeamLetter(extra.team),
                             bot->GetLevel(), bot->GetName().c_str());
                itr = sExtras.erase(itr);
                continue;
            }
            ++itr;
        }
    }

    // The bots this module queued. The core pops a match at the minimum per team and only tops it up to the maximum on
    // a later queue update (BattlegroundQueue::FillPlayersToBG), which nothing schedules while no one joins, so kick
    // the queue every pass while any of them waits. Invited: accept now (FastAccept, BGStatusAction's accept branch).
    // Still uninvited QueueTimeoutSeconds after the last real player of its (queue, bracket) stopped waiting: leave,
    // so leftovers do not start a bots-only match.
    void TendQueued(Config const& cfg, std::map<Key, Demand> const& demands)
    {
        time_t const now = time(nullptr);
        std::set<Key> kick;
        for (auto itr = sOurQueued.begin(); itr != sOurQueued.end();)
        {
            Player* bot = ObjectAccessor::FindConnectedPlayer(itr->first);
            BattlegroundQueueTypeId const queueTypeId = BattlegroundQueueTypeId(itr->second.key.queueTypeId);
            if (!bot || !bot->IsInWorld() || bot->InBattleground() || !bot->InBattlegroundQueueForBattlegroundQueueType(queueTypeId))
            {
                itr = sOurQueued.erase(itr);
                continue;
            }

            if (bot->IsInvitedForBattlegroundQueueType(queueTypeId))
            {
                if (cfg.fastAccept)
                    if (PlayerbotAI* botAI = GET_PLAYERBOT_AI(bot))
                        botAI->DoSpecificAction("bg status", Event(), true);
                ++itr;
                continue;
            }

            if (demands.count(itr->second.key))
                itr->second.lastDemand = now;
            else if (now - itr->second.lastDemand >= time_t(cfg.queueTimeoutSeconds))
            {
                bot->RemoveBattlegroundQueueId(queueTypeId);
                sBattlegroundMgr->GetBattlegroundQueue(queueTypeId).RemovePlayer(bot->GetGUID(), true, true);
                TC_LOG_DEBUG("playerbots", "[BGFILL] %s left queue %u bracket %u (no real player waiting)",
                             bot->GetName().c_str(), uint32(queueTypeId), itr->second.key.bracketId);
                itr = sOurQueued.erase(itr);
                continue;
            }

            kick.insert(itr->second.key);
            ++itr;
        }

        for (Key const& key : kick)
        {
            BattlegroundQueueTypeId const queueTypeId = BattlegroundQueueTypeId(key.queueTypeId);
            sBattlegroundMgr->ScheduleQueueUpdate(0, 0, queueTypeId, BattlegroundMgr::BGTemplateId(queueTypeId),
                                                  BattlegroundBracketId(key.bracketId));
        }
    }

    // Every (queue, bracket) with a real player waiting, with its not-invited queue counts per faction.
    std::map<Key, Demand> CollectDemands(Config const& cfg, bool force)
    {
        std::map<Key, Demand> demands;
        uint32 const nowMs = getMSTime();

        for (uint32 queueType = BATTLEGROUND_QUEUE_AV; queueType < MAX_BATTLEGROUND_QUEUE_TYPES; ++queueType)
        {
            BattlegroundQueueTypeId const queueTypeId = BattlegroundQueueTypeId(queueType);
            if (!IsSupportedBgQueue(queueTypeId) || BattlegroundMgr::BGArenaType(queueTypeId))
                continue;

            BattlegroundTypeId const bgTypeId = BattlegroundMgr::BGTemplateId(queueTypeId);
            Battleground* bgTemplate = sBattlegroundMgr->GetBattlegroundTemplate(bgTypeId);
            if (!bgTemplate)
                continue;

            BattlegroundQueue& queue = sBattlegroundMgr->GetBattlegroundQueue(queueTypeId);
            for (uint32 bracket = 0; bracket < MAX_BATTLEGROUND_BRACKETS; ++bracket)
            {
                uint32 waiting[2] = { 0, 0 };
                uint32 realPlayers = 0;
                uint32 oldestWaitMs = 0;

                for (uint32 type = 0; type < BG_QUEUE_GROUP_TYPES_COUNT; ++type)
                {
                    for (GroupQueueInfo const* group : queue.m_QueuedGroups[bracket][type])
                    {
                        if (!group || group->IsInvitedToBGInstanceGUID || group->IsRated || group->IsRatedBG)
                            continue;

                        uint32 const team = group->Team == ALLIANCE ? TEAM_ALLIANCE : TEAM_HORDE;
                        waiting[team] += uint32(group->Players.size());

                        for (auto const& member : group->Players)
                        {
                            if (!IsRealPlayer(ObjectAccessor::FindConnectedPlayer(member.first)))
                                continue;
                            ++realPlayers;
                            oldestWaitMs = std::max(oldestWaitMs, getMSTimeDiff(group->JoinTime, nowMs));
                        }
                    }
                }

                if (!realPlayers || (!force && oldestWaitMs < cfg.delaySeconds * IN_MILLISECONDS))
                    continue;

                PvPDifficultyEntry const* entry =
                    sDBCManager.GetBattlegroundBracketById(bgTemplate->GetMapId(), BattlegroundBracketId(bracket));
                if (!entry)
                    continue;

                Demand& demand = demands[Key{ queueType, bracket }];
                demand.bgTypeId = bgTypeId;
                demand.bracket = entry;
                demand.minPerTeam = bgTemplate->GetMinPlayersPerTeam();
                uint32 const maxPerTeam = std::max(demand.minPerTeam, bgTemplate->GetMaxPlayersPerTeam());
                demand.target = cfg.teamSize ? std::min(std::max(cfg.teamSize, demand.minPerTeam), maxPerTeam) : maxPerTeam;
                demand.waiting[TEAM_ALLIANCE] = waiting[TEAM_ALLIANCE];
                demand.waiting[TEAM_HORDE] = waiting[TEAM_HORDE];
                demand.realPlayers = realPlayers;
                demand.oldestWaitMs = oldestWaitMs;
            }
        }

        return demands;
    }

    void RunPass(Config const& cfg, bool force)
    {
        std::map<Key, Demand> demands = CollectDemands(cfg, force);
        UpdateExtras(cfg, demands);

        TendQueued(cfg, demands);

        if (demands.empty())
        {
            sLastStatus.clear();
            return;
        }

        // Free random bots, shuffled so the same ones are not always picked.
        std::vector<Player*> bots;
        for (auto const& itr : sRandomPlayerbotMgr->GetAllBots())
            if (itr.second)
                bots.push_back(itr.second);
        static std::mt19937 rng{ std::random_device{}() };
        std::shuffle(bots.begin(), bots.end(), rng);
        std::vector<bool> used(bots.size(), false);

        for (auto& itr : demands)
        {
            Key const& key = itr.first;
            Demand& demand = itr.second;
            BattlegroundQueueTypeId const queueTypeId = BattlegroundQueueTypeId(key.queueTypeId);
            Battleground* bgTemplate = sBattlegroundMgr->GetBattlegroundTemplate(demand.bgTypeId);
            if (!bgTemplate)
                continue;

            bool queuedAny = false;
            for (uint32 team : { uint32(TEAM_ALLIANCE), uint32(TEAM_HORDE) })
            {
                uint32 need = demand.target > demand.waiting[team] ? demand.target - demand.waiting[team] : 0;

                for (size_t i = 0; i < bots.size() && need; ++i)
                {
                    Player* bot = bots[i];
                    if (used[i] || uint32(bot->GetTeamId()) != team || !CanQueue(bot, key, demand, bgTemplate, cfg))
                        continue;
                    if (!QueueBot(bot, queueTypeId, demand, bgTemplate, key))
                        continue;

                    used[i] = true;
                    --need;
                    ++demand.queuedNow[team];
                    ++sTotalQueued;
                    queuedAny = true;

                    auto extra = sExtras.find(bot->GetGUID().GetCounter());
                    if (extra != sExtras.end())
                    {
                        extra->second.queued = true;
                        if (demand.pending[team])
                            --demand.pending[team];
                    }
                }

                // Still short: log in offline bots of this bracket and faction, minus the ones already on their way.
                uint32 toLogin = need > demand.pending[team] ? need - demand.pending[team] : 0;
                if (cfg.loginOffline && toLogin)
                {
                    uint32 const room = cfg.maxExtraLogins > sExtras.size() ? cfg.maxExtraLogins - uint32(sExtras.size()) : 0;
                    toLogin = std::min(toLogin, room);
                    for (auto const& found : FindOfflineBots(demand, team, toLogin))
                    {
                        LoginExtra(found.first, key, team);
                        ++demand.loginsNow[team];
                        ++sTotalLogins;
                    }
                }

                uint32 const covered = demand.pending[team] + demand.loginsNow[team];
                demand.shortfall[team] = need > covered ? need - covered : 0;
            }

            if (queuedAny)
                sBattlegroundMgr->ScheduleQueueUpdate(0, 0, queueTypeId, demand.bgTypeId, demand.bracket->GetBracketId());

            if (queuedAny || demand.loginsNow[TEAM_ALLIANCE] || demand.loginsNow[TEAM_HORDE])
                TC_LOG_INFO("playerbots",
                            "[BGFILL] queue %u bracket %u (lvl %u-%u, %u real): A waiting %u +%u queued +%u login, "
                            "H waiting %u +%u queued +%u login, target %u",
                            key.queueTypeId, key.bracketId, uint32(demand.bracket->MinLevel), uint32(demand.bracket->MaxLevel),
                            demand.realPlayers, demand.waiting[TEAM_ALLIANCE], demand.queuedNow[TEAM_ALLIANCE],
                            demand.loginsNow[TEAM_ALLIANCE], demand.waiting[TEAM_HORDE], demand.queuedNow[TEAM_HORDE],
                            demand.loginsNow[TEAM_HORDE], demand.target);
        }

        sLastStatus.clear();
        for (auto const& itr : demands)
        {
            Demand const& d = itr.second;
            std::ostringstream line;
            line << "queue " << itr.first.queueTypeId << " bracket " << itr.first.bracketId << " (lvl "
                 << uint32(d.bracket->MinLevel) << "-" << uint32(d.bracket->MaxLevel) << "), " << d.realPlayers
                 << " real, waited " << d.oldestWaitMs / IN_MILLISECONDS << "s, target " << d.target << " (min "
                 << d.minPerTeam << ")";
            for (uint32 team : { uint32(TEAM_ALLIANCE), uint32(TEAM_HORDE) })
                line << " | " << TeamLetter(team) << ": waiting " << d.waiting[team] << " +" << d.queuedNow[team]
                     << " queued, " << d.pending[team] << " pending +" << d.loginsNow[team] << " login, short "
                     << d.shortfall[team];
            sLastStatus.push_back(line.str());
        }
    }
}  // namespace

namespace BgQueueFill
{
    void Update(uint32 diff)
    {
        if (!sPlayerbotAIConfig->enabled)
            return;

        Config const cfg = ReadConfig();
        if (!cfg.enable && !sForce)
            return;

        sTimer += diff;
        if (sTimer < cfg.intervalMs && !sForce)
            return;
        sTimer = 0;

        bool const force = sForce;
        sForce = false;
        RunPass(cfg, force);
    }

    void ForceNextPass() { sForce = true; }

    std::string Status()
    {
        Config const cfg = ReadConfig();
        std::ostringstream out;
        out << "bgfill: " << (cfg.enable ? "on" : "off") << ", every " << cfg.intervalMs << " ms, delay "
            << cfg.delaySeconds << " s, team size " << (cfg.teamSize ? std::to_string(cfg.teamSize) : std::string("max"))
            << ", offline logins " << (cfg.loginOffline ? "on" : "off") << " (" << sExtras.size() << "/"
            << cfg.maxExtraLogins << " extra online), fast accept " << (cfg.fastAccept ? "on" : "off")
            << "; since start " << sTotalQueued << " bots queued, " << sTotalLogins << " logged in";
        if (sLastStatus.empty())
            out << "\nno real player is waiting in a battleground queue";
        for (std::string const& line : sLastStatus)
            out << "\n" << line;
        return out.str();
    }
}
