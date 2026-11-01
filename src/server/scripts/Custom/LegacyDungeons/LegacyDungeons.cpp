#include "SoloDungeonPolicy.h"
#include "SoloDungeonHooks.h"
#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "LFGMgr.h"
#include "Log.h"
#include "Map.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "World.h"
#include "WorldSession.h"
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace
{
    struct Trip
    {
        uint32 dungeon, destination, map;
        float x, y, z, o;
        Difficulty difficulty;
    };
    std::mutex tripMutex;
    std::unordered_map<uint32, Trip> trips;
    std::atomic<bool> storageReady{ false };
    rbac::RBACPermissions const Permission = static_cast<rbac::RBACPermissions>(96000);
    rbac::RBACPermissions const ForcePermission = static_cast<rbac::RBACPermissions>(96001);
    bool Enabled() { return sConfigMgr->GetBoolDefault("LFG.LegacySolo.Enable", true); }
    uint32 Margin() { return LegacyDungeons::Margin(sConfigMgr->GetIntDefault("LFG.LegacySolo.LevelsOverCap", 20)); }
    uint32 Cap(lfg::LFGDungeonData const& data) { return LegacyDungeons::OriginalCap(data.minlevel, data.maxlevel); }
    bool Specific(lfg::LFGDungeonData const& data) { return data.type == lfg::LFG_TYPE_DUNGEON || data.type == lfg::LFG_TYPE_HEROIC; }
    bool FiveMan(lfg::LFGDungeonData const& data)
    {
        return data.category == lfg::LFG_CATEGORY_DUNGEON && data.difficulty <= DUNGEON_DIFFICULTY_HEROIC;
    }
    auto Decide(Player* player, lfg::LFGDungeonData const& data, uint32 level, bool force = false)
    {
        return LegacyDungeons::Decide(Enabled(), player->GetSession()->IsBot(), FiveMan(data), Specific(data),
            player->GetGroup() != nullptr, level, Cap(data), Margin(), force);
    }
    void Tell(Player* player, char const* text) { ChatHandler(player->GetSession()).SendSysMessage(text); }
    std::optional<Trip> Find(Player* player)
    {
        std::lock_guard<std::mutex> guard(tripMutex);
        auto found = trips.find(player->GetGUID().GetCounter());
        return found == trips.end() ? std::nullopt : std::optional<Trip>(found->second);
    }
    void Forget(Player* player)
    {
        {
            std::lock_guard<std::mutex> guard(tripMutex);
            trips.erase(player->GetGUID().GetCounter());
        }
        if (storageReady)
            CharacterDatabase.DirectPExecute("DELETE FROM character_legacy_trip WHERE guid=%u", player->GetGUID().GetCounter());
    }
    void Save(Player* player, Trip const& trip)
    {
        // Persist before requesting the transfer; native character save may happen during a disconnect.
        CharacterDatabase.DirectPExecute("REPLACE INTO character_legacy_trip (guid,dungeon,destination,return_map,return_x,return_y,return_z,return_o,difficulty) "
            "VALUES (%u,%u,%u,%u,%f,%f,%f,%f,%u)", player->GetGUID().GetCounter(), trip.dungeon, trip.destination, trip.map,
            trip.x, trip.y, trip.z, trip.o, uint32(trip.difficulty));
        std::lock_guard<std::mutex> guard(tripMutex);
        trips.insert_or_assign(player->GetGUID().GetCounter(), trip);
    }
    void RestoreDifficulty(Player* player, Trip const& trip)
    {
        player->SetDungeonDifficulty(trip.difficulty);
        player->SendDungeonDifficulty(uint32(trip.difficulty));
    }
    void Arrived(Player* player)
    {
        auto trip = Find(player);
        if (!trip || player->GetMapId() == trip->destination || player->InBattleground())
            return;
        // A portal, hearth or completed return transfer also ends this trip.
        if (!player->GetMap()->IsDungeon())
        {
            RestoreDifficulty(player, *trip);
            Forget(player);
            player->SaveToDB();
        }
    }
    void Load(Player* player)
    {
        if (!storageReady || player->GetSession()->IsBot())
            return;
        if (QueryResult result = CharacterDatabase.PQuery("SELECT dungeon,destination,return_map,return_x,return_y,return_z,return_o,difficulty FROM character_legacy_trip WHERE guid=%u", player->GetGUID().GetCounter()))
        {
            Field* row = result->Fetch();
            Trip trip{ row[0].GetUInt32(), row[1].GetUInt32(), row[2].GetUInt32(), row[3].GetFloat(), row[4].GetFloat(), row[5].GetFloat(), row[6].GetFloat(), Difficulty(row[7].GetUInt8()) };
            {
                std::lock_guard<std::mutex> guard(tripMutex);
                trips.insert_or_assign(player->GetGUID().GetCounter(), trip);
            }
            Arrived(player);
        }
    }
    void ProbeStorage()
    {
        storageReady = bool(CharacterDatabase.Query("SHOW TABLES LIKE 'character_legacy_trip'"));
        if (Enabled() && !storageReady)
            TC_LOG_ERROR("server.loading", "Legacy solo dungeons: apply the character_legacy_trip update before enabling travel");
    }
    bool Safe(Player* player, bool entering)
    {
        if (!player->IsInWorld() || (entering && !player->IsAlive()) || player->IsInCombat() || player->IsInFlight() ||
            player->IsFalling() || player->GetVehicle() || player->GetCharmGUID() || player->IsBeingTeleported() ||
            player->InBattleground() || (entering && player->InBattlegroundQueue()))
        {
            Tell(player, "Legacy travel requires a safe stop outside combat, flight and vehicles.");
            return false;
        }
        return true;
    }
    bool Enter(Player* player, uint32 id, bool force = false)
    {
        auto data = sLFGMgr->GetLFGDungeon(id);
        if (!data || !Specific(*data) || !FiveMan(*data))
        {
            Tell(player, "Choose a specific normal or heroic five-player dungeon.");
            return false;
        }
        auto decision = Decide(player, *data, player->GetLevel(), force);
        if (decision != LegacyDungeons::Decision::Solo)
        {
            Tell(player, decision == LegacyDungeons::Decision::RequireSolo ? "Leave your group before using solo dungeon travel." :
                "Solo travel is not unlocked for this dungeon. Legacy bot parties await the instance pool.");
            return false;
        }
        if (!storageReady)
        {
            Tell(player, "Legacy dungeon travel is unavailable. Ask a GM.");
            return false;
        }
        if (!Safe(player, true))
            return false;
        if (player->GetMap()->IsDungeon())
        {
            Tell(player, "Return outside before entering another legacy dungeon.");
            return false;
        }
        uint32 queue = sLFGMgr->GetActiveQueueId(player->GetGUID());
        if (queue && sLFGMgr->GetState(player->GetGUID(), queue) != lfg::LFG_STATE_NONE)
        {
            Tell(player, "Leave your Dungeon Finder queue before using solo travel.");
            return false;
        }
        sLFGMgr->InitializeLockedDungeons(player);
        auto const& locks = sLFGMgr->GetLockedDungeons(player->GetGUID());
        auto lock = locks.find(data->Entry());
        if (lock != locks.end() && lock->second != lfg::LFG_LOCKSTATUS_TOO_HIGH_LEVEL && !(force && lock->second == lfg::LFG_LOCKSTATUS_TOO_LOW_LEVEL))
        {
            Tell(player, "This dungeon is locked. Check its Dungeon Finder requirements.");
            return false;
        }
        if (data->x == 0.0f && data->y == 0.0f && data->z == 0.0f)
        {
            Tell(player, "This dungeon's entrance is unavailable. Ask a GM.");
            return false;
        }
        Trip trip{ id, data->map, player->GetMapId(), player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), player->GetOrientation(), player->GetDungeonDifficulty() };
        Save(player, trip);
        player->SetDungeonDifficulty(data->difficulty);
        player->SendDungeonDifficulty(uint32(data->difficulty));
        if (!player->TeleportTo(data->map, data->x, data->y, data->z, data->o))
        {
            RestoreDifficulty(player, trip);
            Forget(player);
            Tell(player, "The dungeon could not be entered. Its normal access rules still apply.");
            return false;
        }
        Tell(player, "Entering solo. Use .legacy out to return to this spot.");
        return true;
    }
    bool Out(Player* player)
    {
        auto trip = Find(player);
        if (!trip || player->GetMapId() != trip->destination)
        {
            Tell(player, "You are not in an active legacy solo dungeon.");
            return false;
        }
        if (!Safe(player, false))
            return false;
        // Keep the anchor until the worldport arrives; failed/interrupted transfers remain recoverable.
        return player->TeleportTo(trip->map, trip->x, trip->y, trip->z, trip->o);
    }
    bool Unlock(Player* player, lfg::LFGDungeonData const& data, uint32 level)
    {
        return storageReady && Decide(player, data, level) == LegacyDungeons::Decision::Solo;
    }
    bool Join(Player* player, lfg::LfgDungeonSet const& dungeons)
    {
        if (!Enabled() || player->GetSession()->IsBot())
            return false;
        for (uint32 id : dungeons)
            if (auto data = sLFGMgr->GetLFGDungeon(id))
                if (Decide(player, *data, player->GetLevel()) != LegacyDungeons::Decision::Native)
                {
                    bool success = false;
                    if (dungeons.size() == 1)
                        success = Enter(player, id);
                    else
                        Tell(player, "Choose one specific dungeon for solo travel.");
                    player->GetSession()->SendLfgJoinResult(0, lfg::LfgJoinResultData(success ? lfg::LFG_JOIN_OK : lfg::LFG_JOIN_DUNGEON_INVALID));
                    return true;
                }
        return false;
    }
    bool Teleport(Player* player, bool out)
    {
        if (!Find(player))
            return false;
        if (out)
            Out(player);
        else
            Tell(player, "You are already in a legacy solo run. Use .legacy out to return.");
        return true;
    }
    bool OriginalDifficulty(Map const* map)
    {
        std::lock_guard<std::mutex> guard(tripMutex);
        if (trips.empty())
            return false;
        for (auto const& ref : map->GetPlayers())
            if (Player* player = ref.GetSource())
            {
                auto found = trips.find(player->GetGUID().GetCounter());
                if (found != trips.end() && found->second.destination == map->GetId())
                    return true;
            }
        return false;
    }
    lfg::SoloDungeon::Hooks const hooks{ &Unlock, &Join, &Teleport, &OriginalDifficulty };
    Player* Target(ChatHandler* handler)
    {
        if (handler->HasPermission(ForcePermission))
            if (auto player = handler->getSelectedPlayer())
                return player;
        return handler->GetSession()->GetPlayer();
    }
    bool Status(ChatHandler* handler, char const* args)
    {
        Player* player = Target(handler);
        auto trip = Find(player);
        handler->PSendSysMessage("Legacy solo: enabled=%u; storage=%u; margin=%u; grouped=%u; active-dungeon=%u; destination=%u; current-map=%u",
            uint32(Enabled()), uint32(storageReady.load()), Margin(), uint32(player->GetGroup() != nullptr), trip ? trip->dungeon : 0, trip ? trip->destination : 0, player->GetMapId());
        if (auto id = LegacyDungeons::DungeonId(args ? args : ""))
            if (auto data = sLFGMgr->GetLFGDungeon(*id))
                handler->PSendSysMessage("Dungeon %u: %s; original-cap=%u; solo-at=%u; decision=%u", *id, data->name.c_str(), Cap(*data), Cap(*data) + Margin(), uint32(Decide(player, *data, player->GetLevel())));
        return true;
    }
    bool List(ChatHandler* handler, char const*)
    {
        Player* player = Target(handler);
        for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows(); ++i)
            if (auto entry = sLFGDungeonStore.LookupEntry(i))
                if (auto data = sLFGMgr->GetLFGDungeon(entry->ID))
                    if (Decide(player, *data, player->GetLevel()) == LegacyDungeons::Decision::Solo)
                        handler->PSendSysMessage("%u: %s (original cap %u)", data->id, data->name.c_str(), Cap(*data));
        return true;
    }
    bool EnterCommand(ChatHandler* handler, char const* args)
    {
        auto id = LegacyDungeons::DungeonId(args ? args : "");
        return id && Enter(Target(handler), *id);
    }
    bool ForceCommand(ChatHandler* handler, char const* args)
    {
        auto id = LegacyDungeons::DungeonId(args ? args : "");
        return id && Enter(Target(handler), *id, true);
    }
    bool OutCommand(ChatHandler* handler, char const*) { return Out(Target(handler)); }
    class legacy_commands : public CommandScript
    {
    public:
        legacy_commands() : CommandScript("legacy_commands") { }
        Trinity::ChatCommands::ChatCommandTable GetCommands() const override
        {
            using namespace Trinity::ChatCommands;
            static ChatCommandTable commands =
            {
                { "list", &List, Permission, Console::No }, { "status", &Status, Permission, Console::No },
                { "enter", &EnterCommand, Permission, Console::No }, { "out", &OutCommand, Permission, Console::No },
                { "force", &ForceCommand, ForcePermission, Console::No }
            };
            return { { "legacy", commands, Permission, Console::No } };
        }
    };
    bool AnyAtLevel(Player* player, uint32 level)
    {
        for (uint32 i = 0; i < sLFGDungeonStore.GetNumRows(); ++i)
            if (auto entry = sLFGDungeonStore.LookupEntry(i))
                if (auto data = sLFGMgr->GetLFGDungeon(entry->ID))
                    if (Decide(player, *data, level) == LegacyDungeons::Decision::Solo)
                        return true;
        return false;
    }
    void Intro(Player* player)
    {
        if (!player->GetSession()->IsBot() && sConfigMgr->GetBoolDefault("LFG.LegacySolo.IntroOnLogin", true) && AnyAtLevel(player, player->GetLevel()))
            player->GetSession()->SendNotification("Outleveled dungeons can be entered solo. Use Dungeon Finder or .legacy list; .legacy out returns you.");
    }
    class legacy_player : public PlayerScript
    {
    public:
        legacy_player() : PlayerScript("legacy_player") { }
        void OnLogin(Player* player) override { Load(player); Intro(player); }
        void OnLogout(Player* player) override
        {
            std::lock_guard<std::mutex> guard(tripMutex);
            trips.erase(player->GetGUID().GetCounter());
        }
        void OnMapChanged(Player* player) override { Arrived(player); }
        void OnLevelChanged(Player* player, uint8 oldLevel) override
        {
            if (!AnyAtLevel(player, oldLevel))
                Intro(player);
        }
    };
    class legacy_world : public WorldScript
    {
    public:
        legacy_world() : WorldScript("legacy_world") { lfg::SoloDungeon::Install(&hooks); }
        ~legacy_world() override { lfg::SoloDungeon::Install(nullptr); }
        void OnStartup() override { ProbeStorage(); }
        void OnConfigLoad(bool reload) override
        {
            if (reload)
            {
                ProbeStorage();
                for (auto const& session : sWorld->GetAllSessions())
                    if (auto player = session.second->GetPlayer())
                        sLFGMgr->InitializeLockedDungeons(player);
            }
        }
    };
}
void AddSC_legacy_dungeons()
{
    new legacy_world();
    new legacy_player();
    new legacy_commands();
}
