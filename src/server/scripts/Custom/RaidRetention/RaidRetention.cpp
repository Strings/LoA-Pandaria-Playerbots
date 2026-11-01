#include "RaidRetention.h"
#include "RaidResetPolicy.h"
#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Group.h"
#include "InstanceSaveMgr.h"
#include "InstanceScript.h"
#include "Map.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldSession.h"
#include <set>
#include <sstream>

namespace
{
    bool Fail(ChatHandler* handler, char const* message)
    {
        handler->SendSysMessage(message);
        handler->SetSentErrorMessage(true);
        return false;
    }
    bool Parse(char const* args, uint32& id)
    {
        std::istringstream input(args ? args : "");
        std::string extra;
        return bool(input >> id) && !(input >> extra) && id;
    }
    bool Status(ChatHandler* handler, char const* args)
    {
        if (args && *args)
            return false;
        handler->PSendSysMessage("Raid retention: enabled=%u; period=%u seconds", uint32(RaidRetention::Enabled()), RaidRetention::PeriodSeconds());
        return true;
    }
    bool List(ChatHandler* handler, char const* args)
    {
        if (args && *args)
            return false;
        Player* player = handler->GetSession()->GetPlayer();
        if (handler->HasPermission(rbac::RBAC_PERM_COMMAND_INSTANCE_LISTBINDS))
            if (Player* selected = handler->getSelectedPlayer())
                player = selected;
        std::set<uint32> ids;
        for (uint32 difficulty = 0; difficulty < MAX_DIFFICULTY; ++difficulty)
        {
            for (auto const& bind : player->GetBoundInstances(Difficulty(difficulty)))
                ids.insert(bind.second.save->GetInstanceId());
            if (Group* group = player->GetGroup())
                for (auto const& bind : group->GetBoundInstances(Difficulty(difficulty)))
                    ids.insert(bind.second.save->GetInstanceId());
        }
        uint32 count = 0;
        for (uint32 id : ids)
            if (QueryResult row = CharacterDatabase.PQuery("SELECT map,difficulty,completedEncounters,resettime FROM instance WHERE id=%u", id))
            {
                Field* fields = row->Fetch();
                uint32 map = fields[0].GetUInt32();
                MapEntry const* entry = sMapStore.LookupEntry(map);
                if (!entry || !entry->IsRaid())
                    continue;
                ++count;
                Difficulty difficulty = Difficulty(fields[1].GetUInt32());
                uint32 completed = fields[2].GetUInt32();
                handler->PSendSysMessage("Raid bind %u: map=%u; difficulty=%u; progress=%u; expected=%u; finished=%u; script-progress=%u; retained=%u; next=%u",
                    id, map, uint32(difficulty), completed, RaidRetention::ExpectedMask(map, difficulty),
                    uint32(RaidResetPolicy::Finished(fields[3].GetUInt32())), uint32(fields[3].GetUInt32() == RaidResetPolicy::PartialMarker), uint32(RaidRetention::Enabled() && RaidResetPolicy::Preserve(RaidResetPolicy::HasProgress(completed, fields[3].GetUInt32()), RaidResetPolicy::Finished(fields[3].GetUInt32()))), uint32(sInstanceSaveMgr->GetResetTimeFor(map, difficulty)));
            }
        handler->PSendSysMessage("Raid bind IDs: %u", count);
        return true;
    }
    bool Abandon(ChatHandler* handler, char const* args)
    {
        uint32 id;
        if (!Parse(args, id))
            return Fail(handler, "Usage: raidprogress abandon <instance id> (clears this raid and ALL its binds)");
        if (!RaidRetention::Enabled())
            return Fail(handler, "Raid retention is disabled");
        Player* player = handler->GetSession()->GetPlayer();
        Group* group = player->GetGroup();
        bool bound = false;
        for (uint32 difficulty = 0; difficulty < MAX_DIFFICULTY; ++difficulty)
        {
            for (auto const& bind : player->GetBoundInstances(Difficulty(difficulty)))
                bound |= bind.second.save->GetInstanceId() == id;
            if (group)
                for (auto const& bind : group->GetBoundInstances(Difficulty(difficulty)))
                    bound |= bind.second.save->GetInstanceId() == id;
        }
        if (!RaidResetPolicy::MayAbandon(bound, group != nullptr, group && group->IsLeader(player->GetGUID()), player->GetSession()->IsBot()))
            return Fail(handler, "Only the bound group's leader or an ungrouped owner of the bind may abandon. No other members' consent is needed.");
        if (!sInstanceSaveMgr->ResetRaidInstance(id))
            return Fail(handler, "Raid instance not found");
        handler->PSendSysMessage("Abandoned raid %u and cleared all player/group binds", id);
        return true;
    }
    bool Force(ChatHandler* handler, char const* args)
    {
        uint32 id;
        if (args && std::string(args) == "current" && handler->GetSession())
            id = handler->GetSession()->GetPlayer()->GetInstanceId();
        else if (!Parse(args, id))
            return Fail(handler, "Usage: raidprogress force-reset|force-abandon <instance id|current>");
        if (!RaidRetention::Enabled())
            return Fail(handler, "Raid retention is disabled");
        if (!sInstanceSaveMgr->ResetRaidInstance(id))
            return Fail(handler, "Raid instance not found (only raid saves are accepted)");
        handler->PSendSysMessage("Forced reset/abandon of raid %u; all binds cleared", id);
        return true;
    }
    bool Credit(ChatHandler* handler, char const* args)
    {
        uint32 entry;
        if (!Parse(args, entry))
            return Fail(handler, "Usage: raidprogress credit <creature entry> (current raid encounter credit)");
        if (!RaidRetention::Enabled())
            return Fail(handler, "Raid retention is disabled");
        Player* player = handler->GetSession()->GetPlayer();
        Map* map = player->GetMap();
        if (!map->IsRaid())
            return Fail(handler, "Enter a raid first");
        InstanceScript* script = static_cast<InstanceMap*>(map)->GetInstanceScript();
        if (!script)
            return Fail(handler, "Raid has no instance script");
        script->UpdateEncounterState(ENCOUNTER_CREDIT_KILL_CREATURE, entry, player);
        handler->PSendSysMessage("Raid %u: credited creature %u; progress=%u", map->GetInstanceId(), entry, script->GetCompletedEncounterMask());
        return true;
    }
    bool Boss(ChatHandler* handler, char const* args)
    {
        std::istringstream input(args ? args : "");
        uint32 index;
        std::string state, extra;
        if (!(input >> index) || ((input >> state) && (input >> extra)))
            return Fail(handler, "Usage: raidprogress boss <index> [not-started|in-progress|done]");
        if (!RaidRetention::Enabled())
            return Fail(handler, "Raid retention is disabled");
        Map* map = handler->GetSession()->GetPlayer()->GetMap();
        InstanceScript* script = map->IsRaid() ? map->ToInstanceMap()->GetInstanceScript() : nullptr;
        if (!script || index >= script->GetEncounterCount())
            return Fail(handler, "Enter a raid and use a valid boss index");
        bool changed = false;
        if (!state.empty())
        {
            if (state != "not-started" && state != "in-progress" && state != "done")
                return Fail(handler, "State must be not-started, in-progress or done");
            changed = script->SetBossState(index, state == "done" ? DONE : state == "in-progress" ? IN_PROGRESS : NOT_STARTED);
        }
        handler->PSendSysMessage("Raid %u boss %u: state=%u; changed=%u; encounters=%u", map->GetInstanceId(), index, uint32(script->GetBossState(index)), uint32(changed), script->GetEncounterCount());
        return true;
    }
    class raid_retention_commandscript : public CommandScript
    {
    public:
        raid_retention_commandscript() : CommandScript("raid_retention_commandscript") { }
        std::vector<ChatCommand> GetCommands() const override
        {
            using Trinity::ChatCommands::Console;
            std::vector<ChatCommand> children =
            {
                { "status", &Status, rbac::RBAC_PERM_COMMAND_HELP, Console::Yes },
                { "list", &List, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
                { "abandon", &Abandon, rbac::RBAC_PERM_COMMAND_HELP, Console::No },
                { "force-abandon", &Force, rbac::RBAC_PERM_COMMAND_INSTANCE_UNBIND, Console::Yes },
                { "force-reset", &Force, rbac::RBAC_PERM_COMMAND_INSTANCE_UNBIND, Console::Yes },
                { "credit", &Credit, rbac::RBAC_PERM_COMMAND_INSTANCE_UNBIND, Console::No },
                { "boss", &Boss, rbac::RBAC_PERM_COMMAND_INSTANCE_UNBIND, Console::No }
            };
            return { { "raidprogress", children, rbac::RBAC_PERM_COMMAND_HELP, Console::Yes } };
        }
    };
    class raid_retention_playerscript : public PlayerScript
    {
    public:
        raid_retention_playerscript() : PlayerScript("raid_retention_playerscript") { }
        void OnMapChanged(Player* player) override
        {
            if (RaidRetention::Enabled() && player->GetMap()->IsRaid() && !player->GetSession()->IsBot() &&
                sConfigMgr->GetBoolDefault("Raid.Retention.NotifyOnEnter", true))
                player->GetSession()->SendNotification("Unfinished raid progress is kept. Use .raidprogress list; only your leader or a solo bind owner can abandon it.");
        }
    };
}
void AddSC_raid_retention()
{
    new raid_retention_commandscript();
    new raid_retention_playerscript();
}
