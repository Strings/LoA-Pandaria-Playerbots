/*
 * Spawn thinning (T-081): imported world data packs open-world mobs so tightly that most pulls bring three or
 * more. At startup, before any grid loads, hostile spawns closer than SpawnThinning.Radius to an older one are
 * left out of the map grids. World data is untouched: the spawns stay in the database and in sObjectMgr, and
 * Radius 0 brings them all back after a restart. See README.md.
 */

#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "SpawnThinningRules.h"
#include "Timer.h"
#include "Util.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>
#include <unordered_set>

static rbac::RBACPermissions const RBAC_PERM_COMMAND_SPAWNTHIN = static_cast<rbac::RBACPermissions>(91009);

namespace
{
    struct ThinningState
    {
        float Radius = 0.0f;
        bool SameEntryOnly = false;
        std::set<uint32> Maps;
        uint32 Candidates = 0;
        std::unordered_set<uint32> Thinned;    // spawn ids left out of the grids
    };

    ThinningState& State()
    {
        static ThinningState state;
        return state;
    }

    // Spawn ids a script, formation, link, waypoint path, pool or game event depends on: never thinned.
    void CollectProtected(std::unordered_set<uint32>& out)
    {
        char const* queries[] =
        {
            "SELECT CAST(`leaderGUID` AS SIGNED) FROM `creature_formations` UNION SELECT CAST(`memberGUID` AS SIGNED) FROM `creature_formations`",
            "SELECT CAST(`guid` AS SIGNED) FROM `linked_respawn` UNION SELECT CAST(`linkedGuid` AS SIGNED) FROM `linked_respawn`",
            "SELECT CAST(-`entryorguid` AS SIGNED) FROM `smart_scripts` WHERE `entryorguid` < 0 AND `source_type` = 0",
            "SELECT CAST(`guid` AS SIGNED) FROM `creature_addon` WHERE `path_id` <> 0",
            "SELECT CAST(`guid` AS SIGNED) FROM `pool_creature`",
            "SELECT CAST(`guid` AS SIGNED) FROM `game_event_creature`",
        };
        for (char const* sql : queries)
            if (QueryResult result = WorldDatabase.Query(sql))
                do
                    out.insert(uint32(result->Fetch()[0].GetInt64()));
                while (result->NextRow());
    }

    bool IsCandidate(CreatureData const& data, CreatureTemplate const* cInfo)
    {
        if (!cInfo || data.ScriptId || cInfo->ScriptID)
            return false;
        // Only mobs that start fights: not critters, service NPCs, rares or bosses.
        if (cInfo->type == CREATURE_TYPE_CRITTER || data.npcflag || data.npcflag2 || cInfo->npcflag || cInfo->npcflag2)
            return false;
        if (cInfo->rank != CREATURE_ELITE_NORMAL && cInfo->rank != CREATURE_ELITE_ELITE)
            return false;
        FactionTemplateEntry const* faction = sFactionTemplateStore.LookupEntry(cInfo->faction);
        return faction && faction->IsHostileToPlayers();
    }

    void Apply()
    {
        ThinningState& state = State();
        state.Radius = sConfigMgr->GetFloatDefault("SpawnThinning.Radius", 0.0f);
        state.SameEntryOnly = sConfigMgr->GetBoolDefault("SpawnThinning.SameEntryOnly", false);
        state.Maps.clear();
        std::string maps = sConfigMgr->GetStringDefault("SpawnThinning.Maps", "0 1 530 571 870");
        for (std::string_view token : Trinity::Tokenize(maps, ' ', false))
            state.Maps.insert(uint32(atoi(std::string(token).c_str())));
        if (state.Radius <= 0.0f || state.Maps.empty())
            return;

        uint32 oldMSTime = getMSTime();
        std::unordered_set<uint32> protectedSpawns;
        CollectProtected(protectedSpawns);

        std::vector<SpawnThinning::Spawn> spawns;
        for (auto const& [spawnId, data] : sObjectMgr->GetAllCreatureData())
        {
            if (!state.Maps.count(data.mapId) || protectedSpawns.count(spawnId))
                continue;
            if (!IsCandidate(data, sObjectMgr->GetCreatureTemplate(data.id)))
                continue;
            SpawnThinning::Spawn spawn;
            spawn.Id = spawnId;
            spawn.SpawnGroup = { data.mapId, data.phaseMask, data.phaseid, data.phaseGroup, state.SameEntryOnly ? data.id : 0 };
            spawn.X = data.posX;
            spawn.Y = data.posY;
            spawns.push_back(spawn);
        }
        std::sort(spawns.begin(), spawns.end(), [](SpawnThinning::Spawn const& a, SpawnThinning::Spawn const& b) { return a.Id < b.Id; });
        state.Candidates = uint32(spawns.size());

        for (uint64_t spawnId : SpawnThinning::SelectThinned(spawns, state.Radius))
            if (CreatureData const* data = sObjectMgr->GetCreatureData(uint32(spawnId)))
            {
                sObjectMgr->RemoveCreatureFromGrid(uint32(spawnId), data);
                state.Thinned.insert(uint32(spawnId));
            }

        TC_LOG_INFO("server.loading", ">> Spawn thinning: left %u of %u hostile open-world spawns out (radius %.1f yd%s) in %u ms",
            uint32(state.Thinned.size()), state.Candidates, state.Radius, state.SameEntryOnly ? ", same entry only" : "", GetMSTimeDiffToNow(oldMSTime));
    }
}

class spawn_thinning_worldscript : public WorldScript
{
public:
    spawn_thinning_worldscript() : WorldScript("spawn_thinning_worldscript") { }

    // Runs after the world data loads and before any player can load a grid.
    void OnStartup() override
    {
        Apply();
    }
};

class spawn_thinning_commandscript : public CommandScript
{
public:
    spawn_thinning_commandscript() : CommandScript("spawn_thinning_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> spawnthinTable =
        {
            { "near", &HandleNearCommand, RBAC_PERM_COMMAND_SPAWNTHIN, Trinity::ChatCommands::Console::No },
            { "",     &HandleInfoCommand, RBAC_PERM_COMMAND_SPAWNTHIN, Trinity::ChatCommands::Console::Yes },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "spawnthin", spawnthinTable, RBAC_PERM_COMMAND_SPAWNTHIN, Trinity::ChatCommands::Console::Yes },
        };
        return commandTable;
    }

    static bool HandleInfoCommand(ChatHandler* handler, char const* /*args*/)
    {
        ThinningState const& state = State();
        if (state.Radius <= 0.0f)
        {
            handler->SendSysMessage("Spawn thinning is off (SpawnThinning.Radius = 0).");
            return true;
        }
        std::ostringstream maps;
        for (uint32 mapId : state.Maps)
            maps << ' ' << mapId;
        handler->PSendSysMessage("Spawn thinning: radius %.1f yd%s, maps%s. %u of %u hostile spawns left out since startup. "
            "Changes apply after a restart; .spawnthin near [yards] lists the ones around you.", state.Radius,
            state.SameEntryOnly ? " (same entry only)" : "", maps.str().c_str(), uint32(state.Thinned.size()), state.Candidates);
        return true;
    }

    static bool HandleNearCommand(ChatHandler* handler, char const* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        float range = 40.0f;
        if (args && *args)
            range = std::max(1.0f, float(atof(args)));

        ThinningState const& state = State();
        uint32 shown = 0, total = 0;
        for (uint32 spawnId : state.Thinned)
        {
            CreatureData const* data = sObjectMgr->GetCreatureData(spawnId);
            if (!data || data->mapId != player->GetMapId())
                continue;
            float distance = std::sqrt(player->GetExactDist2dSq(data->posX, data->posY));
            if (distance > range)
                continue;
            ++total;
            if (shown < 10)
            {
                CreatureTemplate const* cInfo = sObjectMgr->GetCreatureTemplate(data->id);
                handler->PSendSysMessage(" spawn %u: %s (%u), %.1f yd away", spawnId, cInfo ? cInfo->Name.c_str() : "?", data->id, distance);
                ++shown;
            }
        }
        handler->PSendSysMessage("%u thinned spawns within %.0f yd.", total, range);
        return true;
    }
};

void AddSC_spawn_thinning()
{
    new spawn_thinning_worldscript();
    new spawn_thinning_commandscript();
}
