#include "TreasureRules.h"
#include "CellImpl.h"
#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "GameObject.h"
#include "GameObjectHooks.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Language.h"
#include "Log.h"
#include "LootMgr.h"
#include "Map.h"
#include "MapManager.h"
#include "MotionMaster.h"
#include "MMapFactory.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "PathGenerator.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldSession.h"
#include <algorithm>
#include <cmath>
#include <ctime>
#include <list>
#include <map>
#include <shared_mutex>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

// Hand-placed world treasures (T-093): chests in nooks that open once per character and then vanish for that
// character only, plus the server-side placement validator (DESIGN.md, World placement tool step 3).
namespace
{
    rbac::RBACPermissions const GmPermission = static_cast<rbac::RBACPermissions>(98100);
    char const* const TreasureScript = "go_treasure";
    uint32 const ProbeCreatureEntry = 1; // "Waypoint (Only GM can see it)": a never-spawned path probe

    struct Config
    {
        bool Enable = true;
        bool GmSeesLooted = true;
        bool Intro = true;
        Treasures::Limits Limits;
        float SkyRayHeight = 60.0f;
        float NudgeRadius = 6.0f;
        float NudgeStep = 1.5f;
    };
    Config config;

    void LoadConfig()
    {
        config.Enable = sConfigMgr->GetBoolDefault("Treasures.Enable", true);
        config.GmSeesLooted = sConfigMgr->GetBoolDefault("Treasures.GmSeesLooted", true);
        config.Intro = sConfigMgr->GetBoolDefault("Treasures.IntroOnFirstFind", true);
        config.Limits.MaxGroundOffset = sConfigMgr->GetFloatDefault("Treasures.Validate.MaxGroundOffset", 3.0f);
        config.Limits.MaxSlope = sConfigMgr->GetFloatDefault("Treasures.Validate.MaxSlope", 40.0f);
        config.Limits.MinSpacing = sConfigMgr->GetFloatDefault("Treasures.Validate.MinSpacing", 60.0f);
        config.Limits.MaxPathFactor = sConfigMgr->GetFloatDefault("Treasures.Validate.MaxPathFactor", 4.0f);
        config.Limits.RequireCover = sConfigMgr->GetBoolDefault("Treasures.Validate.RequireCover", false);
        config.SkyRayHeight = std::max(5.0f, sConfigMgr->GetFloatDefault("Treasures.Validate.SkyRayHeight", 60.0f));
        config.NudgeRadius = std::max(0.0f, sConfigMgr->GetFloatDefault("Treasures.Validate.NudgeRadius", 6.0f));
        config.NudgeStep = std::max(0.25f, sConfigMgr->GetFloatDefault("Treasures.Validate.NudgeStep", 1.5f));
    }

    // Template script id of go_treasure, resolved once at startup; 0 until then so the filters stay inert.
    uint32 treasureScriptId = 0;

    bool IsTreasure(GameObjectTemplate const* info)
    {
        return info && treasureScriptId && info->ScriptId == treasureScriptId;
    }

    bool IsTreasure(GameObject const* go)
    {
        return go && IsTreasure(go->GetGOInfo());
    }

    // Spot metadata from world.treasure_spot; a treasure spawn without a row still works, it just has no label.
    struct Spot
    {
        std::string Label;
        std::string Landmark;
        bool HasRoad = false;
        float RoadX = 0.0f, RoadY = 0.0f, RoadZ = 0.0f;
    };
    std::map<uint32, Spot> spots;

    void LoadSpots()
    {
        spots.clear();
        QueryResult result = WorldDatabase.Query("SELECT spawn_guid, label, landmark, road_x, road_y, road_z FROM treasure_spot");
        if (!result)
            return;
        do
        {
            Field* f = result->Fetch();
            Spot& spot = spots[f[0].GetUInt32()];
            spot.Label = f[1].GetString();
            spot.Landmark = f[2].GetString();
            spot.RoadX = f[3].GetFloat();
            spot.RoadY = f[4].GetFloat();
            spot.RoadZ = f[5].GetFloat();
            spot.HasRoad = spot.RoadX != 0.0f || spot.RoadY != 0.0f || spot.RoadZ != 0.0f;
        } while (result->NextRow());
    }

    // Looted treasures (gameobject spawn ids) per online character. The hide filter reads this on map threads.
    std::shared_mutex lootedLock;
    std::unordered_map<ObjectGuid::LowType, std::unordered_set<uint32>> looted;

    bool HasLooted(ObjectGuid::LowType character, uint32 spawnId)
    {
        std::shared_lock<std::shared_mutex> guard(lootedLock);
        auto it = looted.find(character);
        return it != looted.end() && it->second.count(spawnId);
    }

    size_t LootedCount(ObjectGuid::LowType character)
    {
        std::shared_lock<std::shared_mutex> guard(lootedLock);
        auto it = looted.find(character);
        return it == looted.end() ? 0 : it->second.size();
    }

    void LoadLooted(ObjectGuid::LowType character)
    {
        std::unordered_set<uint32> ids;
        if (QueryResult result = CharacterDatabase.PQuery("SELECT spawn_id FROM character_treasure WHERE guid = %u", character))
            do
                ids.insert(result->Fetch()[0].GetUInt32());
            while (result->NextRow());
        std::unique_lock<std::shared_mutex> guard(lootedLock);
        looted[character] = std::move(ids);
    }

    bool HideFilter(GameObject const* go, WorldObject const* seer)
    {
        if (!config.Enable || !IsTreasure(go) || !go->GetDBTableGUIDLow())
            return false;
        Player const* player = seer ? seer->ToPlayer() : nullptr;
        if (!player || (config.GmSeesLooted && player->IsGameMaster()))
            return false;
        return HasLooted(player->GetGUID().GetCounter(), go->GetDBTableGUIDLow());
    }

    bool SparkleFilter(GameObject const* go, WorldObject const* seer)
    {
        if (!config.Enable || !IsTreasure(go) || !go->GetDBTableGUIDLow())
            return false;
        Player const* player = seer ? seer->ToPlayer() : nullptr;
        return player && !HasLooted(player->GetGUID().GetCounter(), go->GetDBTableGUIDLow());
    }

    // Re-run visibility for every treasure the player could see, after their looted set changed.
    void RefreshNearby(Player* player)
    {
        if (!player->IsInWorld())
            return;
        std::list<GameObject*> objects;
        Trinity::AllWorldObjectsInRange check(player, player->GetVisibilityRange());
        Trinity::GameObjectListSearcher<Trinity::AllWorldObjectsInRange> searcher(player, objects, check);
        player->VisitNearbyGridObject(player->GetVisibilityRange(), searcher);
        for (GameObject* go : objects)
            if (IsTreasure(go))
                player->UpdateVisibilityOf(go);
    }

    std::string NameOf(GameObject const* go)
    {
        auto it = spots.find(go->GetDBTableGUIDLow());
        return it != spots.end() && !it->second.Label.empty() ? it->second.Label : go->GetGOInfo()->name;
    }

    // Opens a treasure for one character: rolls its loot for them alone, puts it in the bags, records it.
    // A personal roll instead of the chest's shared loot window, so friends can open the same chest at once.
    bool Open(Player* player, GameObject* go)
    {
        uint32 spawnId = go->GetDBTableGUIDLow();
        ChatHandler chat(player->GetSession());
        if (!spawnId)
        {
            chat.SendSysMessage("This treasure was not placed from the database; it holds nothing.");
            return true;
        }
        ObjectGuid::LowType character = player->GetGUID().GetCounter();
        if (HasLooted(character, spawnId))
        {
            chat.SendSysMessage("You already emptied this treasure.");
            return true;
        }

        Loot loot;
        if (uint32 lootId = go->GetGOInfo()->GetLootId())
            loot.FillLoot(go, lootId, LootTemplates_Gameobject, player, true, true);
        if (GameObjectTemplateAddon const* addon = go->GetTemplateAddon())
            loot.generateMoneyLoot(addon->mingold, addon->maxgold);

        std::vector<std::pair<ItemPosCountVec, LootItem const*>> stores;
        for (LootItem const& item : loot.items)
        {
            if (!item.itemid || item.currency || !item.AllowedForPlayer(player, go))
                continue;
            ItemPosCountVec dest;
            if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, item.itemid, item.count) != EQUIP_ERR_OK)
            {
                player->SendEquipError(EQUIP_ERR_INV_FULL, nullptr, nullptr, item.itemid);
                return true;
            }
            stores.emplace_back(std::move(dest), &item);
        }

        bool first = LootedCount(character) == 0;
        {
            std::unique_lock<std::shared_mutex> guard(lootedLock);
            looted[character].insert(spawnId);
        }
        CharacterDatabase.PExecute("REPLACE INTO character_treasure (guid, spawn_id, looted_at) VALUES (%u, %u, %u)",
            character, spawnId, uint32(time(nullptr)));

        for (auto const& store : stores)
            if (Item* item = player->StoreNewItem(store.first, store.second->itemid, true, store.second->randomPropertyId))
                player->SendNewItem(item, store.second->count, false, false, true);
        if (loot.gold)
        {
            player->ModifyMoney(loot.gold);
            WorldPacket data(SMSG_LOOT_MONEY_NOTIFY, 4 + 1);
            data.WriteBit(1); // "You loot..."
            data.FlushBits();
            data << uint32(loot.gold);
            player->SendDirectMessage(&data);
        }

        chat.PSendSysMessage("You open the %s.", NameOf(go).c_str());
        if (first && config.Intro)
            player->GetSession()->SendNotification("Treasure! Each one opens once for each of your characters.");
        TC_LOG_INFO("scripts", "Treasures: %s opened treasure %u (%u items, %u copper)", player->GetName().c_str(),
            spawnId, uint32(stores.size()), loot.gold);
        player->UpdateVisibilityOf(go);
        return true;
    }

    // ---- Placement validation -------------------------------------------------------------------------------

    struct Probe
    {
        float X = 0.0f, Y = 0.0f, Z = 0.0f;   // candidate as given
        float GroundZ = 0.0f;
        bool Outdoors = true;
        bool InWmo = false;
        int32 RootId = 0, GroupId = 0;
        std::string Liquid = "none";
        int RaysBlocked = 0, RaysTotal = 0;
        float PathLength = 0.0f;
        uint32 PathType = 0;
        uint32 NearestId = 0;
        Treasures::Measurement M;
        Treasures::Verdict V;
    };

    float HeightAt(Map* map, float x, float y, float z)
    {
        return map->GetHeight(PHASEMASK_NORMAL, x, y, z + 2.0f, true, 50.0f);
    }

    bool HasHeight(float h)
    {
        return h > INVALID_HEIGHT + 1.0f;
    }

    // Distance to the nearest treasure spawn on the map other than `self`.
    float NearestTreasure(uint32 mapId, float x, float y, float z, uint32 self, uint32& nearestId)
    {
        float best = -1.0f;
        nearestId = 0;
        for (auto const& pair : sObjectMgr->GetAllGOData())
        {
            GameObjectData const& data = pair.second;
            if (pair.first == self || data.mapid != mapId || !IsTreasure(sObjectMgr->GetGameObjectTemplate(data.id)))
                continue;
            float d = std::sqrt((data.posX - x) * (data.posX - x) + (data.posY - y) * (data.posY - y) + (data.posZ - z) * (data.posZ - z));
            if (best < 0.0f || d < best)
            {
                best = d;
                nearestId = pair.first;
            }
        }
        return best;
    }

    void LoadGridsAlong(Map* map, float x1, float y1, float x2, float y2)
    {
        float length = std::sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
        int steps = std::max(1, int(length / 100.0f));
        for (int i = 0; i <= steps; ++i)
            map->LoadGrid(x1 + (x2 - x1) * i / steps, y1 + (y2 - y1) * i / steps);
    }

    // Measures one candidate. The road point is where a player walks from; `self` is the spot's own spawn id when
    // re-checking a placed treasure, so it doesn't count as its own neighbour.
    Probe Measure(Map* map, float x, float y, float z, bool hasRoad, float rx, float ry, float rz, uint32 self)
    {
        Probe p;
        p.X = x; p.Y = y; p.Z = z;
        LoadGridsAlong(map, hasRoad ? rx : x, hasRoad ? ry : y, x, y);

        float ground = HeightAt(map, x, y, z);
        p.M.GroundFound = HasHeight(ground);
        p.GroundZ = p.M.GroundFound ? ground : z;
        p.M.GroundOffset = std::fabs(z - p.GroundZ);
        float gz = p.GroundZ;

        uint32 mogpFlags = 0;
        int32 adtId = 0;
        p.InWmo = map->GetAreaInfo(PHASEMASK_NORMAL, x, y, gz + 0.5f, mogpFlags, adtId, p.RootId, p.GroupId);
        p.Outdoors = map->IsOutdoors(x, y, gz + 0.5f);

        LiquidData liquid;
        ZLiquidStatus status = map->GetLiquidStatus(PHASEMASK_NORMAL, x, y, gz + 0.1f, MAP_ALL_LIQUIDS, &liquid);
        if (status & (LIQUID_MAP_IN_WATER | LIQUID_MAP_UNDER_WATER))
        {
            float depth = liquid.level - gz;
            p.Liquid = depth > 0.8f ? "deep" : "shallow";
            p.M.Submerged = depth > 0.8f;
        }

        float step = 1.0f;
        float e = HeightAt(map, x + step, y, gz), w = HeightAt(map, x - step, y, gz);
        float n = HeightAt(map, x, y + step, gz), s = HeightAt(map, x, y - step, gz);
        p.M.Slope = (HasHeight(e) && HasHeight(w) && HasHeight(n) && HasHeight(s)) ? Treasures::SlopeDegrees(e, w, n, s, step) : 90.0f;

        // Sky: one ray straight up and four from points a yard and a half out (DESIGN.md, Flying rule).
        float const offsets[5][2] = { { 0.0f, 0.0f }, { 1.5f, 0.0f }, { -1.5f, 0.0f }, { 0.0f, 1.5f }, { 0.0f, -1.5f } };
        for (auto const& o : offsets)
        {
            float sz = gz + 1.0f;
            ++p.RaysTotal;
            if (!map->isInLineOfSight(x + o[0], y + o[1], sz, x + o[0], y + o[1], sz + config.SkyRayHeight, PHASEMASK_NORMAL, VMAP::ModelIgnoreFlags::Nothing))
                ++p.RaysBlocked;
        }
        p.M.SkyState = Treasures::SkyFromRays(p.RaysBlocked, p.RaysTotal, !p.Outdoors);

        if (hasRoad)
        {
            p.M.StraightDistance = std::sqrt((x - rx) * (x - rx) + (y - ry) * (y - ry) + (gz - rz) * (gz - rz));
            p.M.LineOfSight = map->isInLineOfSight(rx, ry, rz + 2.0f, x, y, gz + 0.5f, PHASEMASK_NORMAL, VMAP::ModelIgnoreFlags::Nothing);
            if (MMAP::MMapFactory::createOrGetMMapManager()->GetNavMesh(map->GetId()))
            {
                // The same PathGenerator `.mmap path` uses, run from an unspawned probe standing on the road point.
                Creature* probe = new Creature();
                if (probe->Create(map->GenerateLowGuid<HighGuid::Unit>(), map, PHASEMASK_NORMAL, ProbeCreatureEntry, 0, 0, rx, ry, rz, 0.0f))
                {
                    PathGenerator path(probe);
                    bool built = path.CalculatePath(x, y, gz, false, false);
                    p.PathType = path.GetPathType();
                    Movement::PointsArray const& points = path.GetPath();
                    for (size_t i = 1; i < points.size(); ++i)
                        p.PathLength += (points[i] - points[i - 1]).length();
                    G3D::Vector3 end = path.GetActualEndPosition();
                    float miss = std::sqrt((end.x - x) * (end.x - x) + (end.y - y) * (end.y - y) + (end.z - gz) * (end.z - gz));
                    p.M.Reachable = built && (p.PathType & PATHFIND_NORMAL) && !(p.PathType & PATHFIND_NOPATH) && miss < 3.0f;
                    p.M.PathLength = p.PathLength;
                }
                delete probe;
            }
        }
        else
            p.M.Reachable = true; // no access point given: reachability not judged

        p.M.NearestTreasure = NearestTreasure(map->GetId(), x, y, gz, self, p.NearestId);
        p.V = Treasures::Judge(p.M, config.Limits);
        return p;
    }

    // Tries the candidate, then rings around it, and returns the best passing probe (or the original if none pass).
    Probe MeasureWithNudge(Map* map, float x, float y, float z, bool hasRoad, float rx, float ry, float rz, uint32 self, bool nudge)
    {
        Probe original = Measure(map, x, y, z, hasRoad, rx, ry, rz, self);
        if (!nudge || original.V.Passed())
            return original;
        for (auto const& offset : Treasures::NudgeOffsets(config.NudgeRadius, config.NudgeStep))
        {
            float nx = x + offset.first, ny = y + offset.second;
            float h = HeightAt(map, nx, ny, z);
            Probe candidate = Measure(map, nx, ny, HasHeight(h) ? h : z, hasRoad, rx, ry, rz, self);
            if (candidate.V.Passed())
                return candidate;
        }
        return original;
    }

    void Report(ChatHandler* handler, Probe const& p, uint32 mapId, char const* prefix)
    {
        handler->PSendSysMessage("%s(%.2f, %.2f, %.2f) map %u: %s, score %d", prefix, p.X, p.Y, p.GroundZ, mapId,
            p.V.Passed() ? "PASS" : ("FAIL: " + Treasures::Join(p.V.Failures)).c_str(), p.V.Score);
        handler->PSendSysMessage("  ground %s (offset %.1f), %s%s, liquid %s, slope %.0f deg",
            p.M.GroundFound ? "found" : "missing", p.M.GroundOffset, p.Outdoors ? "outdoors" : "indoors",
            p.InWmo ? (" (WMO " + std::to_string(p.RootId) + "/" + std::to_string(p.GroupId) + ")").c_str() : "",
            p.Liquid.c_str(), p.M.Slope);
        handler->PSendSysMessage("  sky %s (%d/%d rays blocked); path %s %.0f yd (straight %.0f, type 0x%X); line of sight %s",
            Treasures::SkyName(p.M.SkyState), p.RaysBlocked, p.RaysTotal, p.M.Reachable ? "ok" : "none", p.M.PathLength,
            p.M.StraightDistance, p.PathType, p.M.LineOfSight ? "yes" : "no");
        if (p.M.NearestTreasure >= 0.0f)
            handler->PSendSysMessage("  nearest treasure %u at %.0f yd; warnings: %s", p.NearestId, p.M.NearestTreasure,
                p.V.Warnings.empty() ? "none" : Treasures::Join(p.V.Warnings).c_str());
        else
            handler->PSendSysMessage("  no other treasure on this map; warnings: %s",
                p.V.Warnings.empty() ? "none" : Treasures::Join(p.V.Warnings).c_str());
    }

    void Startup()
    {
        treasureScriptId = sObjectMgr->GetScriptId(TreasureScript);
        LoadSpots();
        uint32 count = 0;
        for (auto const& pair : sObjectMgr->GetAllGOData())
            if (IsTreasure(sObjectMgr->GetGameObjectTemplate(pair.second.id)))
                ++count;
        TC_LOG_INFO("server.loading", ">> Treasures: %u spawns, %u spot rows", count, uint32(spots.size()));
    }
}

class go_treasure : public GameObjectScript
{
public:
    go_treasure() : GameObjectScript(TreasureScript) { }

    bool OnGossipHello(Player* player, GameObject* go) override
    {
        if (!config.Enable)
            return false;
        return Open(player, go);
    }
};

class treasures_playerscript : public PlayerScript
{
public:
    treasures_playerscript() : PlayerScript("treasures_playerscript") { }

    void OnLogin(Player* player) override
    {
        LoadLooted(player->GetGUID().GetCounter());
        // The map sent nearby objects before this hook; hide what this character already looted.
        RefreshNearby(player);
    }

    void OnLogout(Player* player) override
    {
        std::unique_lock<std::shared_mutex> guard(lootedLock);
        looted.erase(player->GetGUID().GetCounter());
    }

    void OnDelete(ObjectGuid guid) override
    {
        CharacterDatabase.PExecute("DELETE FROM character_treasure WHERE guid = %u", guid.GetCounter());
    }
};

class treasures_worldscript : public WorldScript
{
public:
    treasures_worldscript() : WorldScript("treasures_worldscript") { }

    void OnConfigLoad(bool /*reload*/) override { LoadConfig(); }
    void OnStartup() override { Startup(); }
};

class treasures_commandscript : public CommandScript
{
public:
    treasures_commandscript() : CommandScript("treasures_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> treasureTable =
        {
            { "list",     &HandleList,     GmPermission, Trinity::ChatCommands::Console::Yes },
            { "go",       &HandleGo,       GmPermission, Trinity::ChatCommands::Console::No },
            { "spawn",    &HandleSpawn,    GmPermission, Trinity::ChatCommands::Console::No },
            { "reset",    &HandleReset,    GmPermission, Trinity::ChatCommands::Console::Yes },
            { "explain",  &HandleExplain,  GmPermission, Trinity::ChatCommands::Console::Yes },
            { "looted",   &HandleLooted,   GmPermission, Trinity::ChatCommands::Console::Yes },
            { "validate", &HandleValidate, GmPermission, Trinity::ChatCommands::Console::Yes },
            { "check",    &HandleCheck,    GmPermission, Trinity::ChatCommands::Console::Yes },
            { "reload",   &HandleReload,   GmPermission, Trinity::ChatCommands::Console::Yes },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "treasure", treasureTable, GmPermission, Trinity::ChatCommands::Console::Yes },
        };
        return commandTable;
    }

    static std::vector<std::string> Words(char const* args)
    {
        std::vector<std::string> words;
        std::istringstream in(args ? args : "");
        std::string word;
        while (in >> word)
            words.push_back(word);
        return words;
    }

    static bool ParseFloat(std::string const& text, float& out)
    {
        try { size_t used = 0; out = std::stof(text, &used); return used == text.size(); }
        catch (...) { return false; }
    }

    static std::vector<std::pair<uint32, GameObjectData const*>> AllTreasures(int32 mapFilter = -1)
    {
        std::vector<std::pair<uint32, GameObjectData const*>> list;
        for (auto const& pair : sObjectMgr->GetAllGOData())
            if ((mapFilter < 0 || pair.second.mapid == uint32(mapFilter)) && IsTreasure(sObjectMgr->GetGameObjectTemplate(pair.second.id)))
                list.emplace_back(pair.first, &pair.second);
        std::sort(list.begin(), list.end(), [](auto const& a, auto const& b) { return a.first < b.first; });
        return list;
    }

    static std::string Label(uint32 spawnId, GameObjectData const* data)
    {
        auto it = spots.find(spawnId);
        if (it != spots.end() && !it->second.Label.empty())
            return it->second.Label;
        GameObjectTemplate const* info = sObjectMgr->GetGameObjectTemplate(data->id);
        return info ? info->name : "?";
    }

    // .treasure list [map]
    static bool HandleList(ChatHandler* handler, char const* args)
    {
        int32 mapFilter = args && *args ? atoi(args) : -1;
        auto list = AllTreasures(mapFilter);
        handler->PSendSysMessage("%u treasures%s:", uint32(list.size()), mapFilter >= 0 ? (" on map " + std::to_string(mapFilter)).c_str() : "");
        for (auto const& t : list)
        {
            std::string landmark = spots.count(t.first) ? spots[t.first].Landmark : "";
            handler->PSendSysMessage("  %u: %s (entry %u) map %u at %.1f %.1f %.1f%s%s", t.first, Label(t.first, t.second).c_str(),
                t.second->id, t.second->mapid, t.second->posX, t.second->posY, t.second->posZ,
                landmark.empty() ? "" : " - ", landmark.c_str());
        }
        return true;
    }

    static GameObjectData const* Find(ChatHandler* handler, uint32 spawnId)
    {
        GameObjectData const* data = sObjectMgr->GetGOData(spawnId);
        if (!data || !IsTreasure(sObjectMgr->GetGameObjectTemplate(data->id)))
        {
            handler->PSendSysMessage("No treasure with spawn id %u.", spawnId);
            return nullptr;
        }
        return data;
    }

    // .treasure go <id>
    static bool HandleGo(ChatHandler* handler, char const* args)
    {
        uint32 spawnId = args ? uint32(atoi(args)) : 0;
        GameObjectData const* data = Find(handler, spawnId);
        if (!data)
            return true;
        Player* player = handler->GetSession()->GetPlayer();
        if (player->IsInFlight())
        {
            player->GetMotionMaster()->MovementExpired();
            player->CleanupAfterTaxiFlight();
        }
        else
            player->SaveRecallPosition();
        player->TeleportTo(data->mapid, data->posX, data->posY, data->posZ + 0.5f, player->GetOrientation());
        handler->PSendSysMessage("Going to treasure %u, %s.", spawnId, Label(spawnId, data).c_str());
        return true;
    }

    // .treasure spawn <entry> [roadX roadY roadZ]: places a permanent treasure at your feet after validating it.
    static bool HandleSpawn(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> w = Words(args);
        uint32 entry = w.empty() ? 0 : uint32(atoi(w[0].c_str()));
        GameObjectTemplate const* info = sObjectMgr->GetGameObjectTemplate(entry);
        if (!IsTreasure(info))
        {
            handler->PSendSysMessage("Entry %u is not a treasure template (ScriptName %s).", entry, TreasureScript);
            return true;
        }
        Player* player = handler->GetSession()->GetPlayer();
        Map* map = player->GetMap();
        float rx = 0.0f, ry = 0.0f, rz = 0.0f;
        bool hasRoad = w.size() >= 4 && ParseFloat(w[1], rx) && ParseFloat(w[2], ry) && ParseFloat(w[3], rz);
        Probe p = Measure(map, player->GetPositionX(), player->GetPositionY(), player->GetPositionZ(), hasRoad, rx, ry, rz, 0);
        Report(handler, p, map->GetId(), "Spot ");
        if (!p.V.Passed())
        {
            handler->SendSysMessage("Not spawned: fix the spot or nudge it with .treasure validate ... nudge.");
            return true;
        }

        GameObject* object = new GameObject;
        if (!object->Create(map->GenerateLowGuid<HighGuid::GameObject>(), entry, map, PHASEMASK_NORMAL, p.X, p.Y, p.GroundZ,
            player->GetOrientation(), { }, 0, GO_STATE_READY))
        {
            delete object;
            return true;
        }
        object->SaveToDB(map->GetId(), (1 << map->GetSpawnMode()), PHASEMASK_NORMAL);
        uint32 spawnId = object->GetDBTableGUIDLow();
        if (!object->LoadGameObjectFromDB(spawnId, map))
        {
            delete object;
            return true;
        }
        sObjectMgr->AddGameobjectToGrid(spawnId, sObjectMgr->GetGOData(spawnId));
        WorldDatabase.PExecute("REPLACE INTO treasure_spot (spawn_guid, label, landmark, road_x, road_y, road_z) VALUES (%u, '', '', %f, %f, %f)",
            spawnId, rx, ry, rz);
        Spot& spot = spots[spawnId];
        spot.HasRoad = hasRoad;
        spot.RoadX = rx; spot.RoadY = ry; spot.RoadZ = rz;
        handler->PSendSysMessage("Treasure %u (%s) placed. Label it in world.treasure_spot.", spawnId, info->name.c_str());
        return true;
    }

    // Character by name (online or not) or, in game, the selected player or yourself.
    static bool Character(ChatHandler* handler, std::string const& name, ObjectGuid& guid, std::string& outName)
    {
        if (!name.empty())
        {
            std::string normalized = name;
            if (!normalizePlayerName(normalized))
                return false;
            guid = sObjectMgr->GetPlayerGUIDByName(normalized);
            outName = normalized;
            return !guid.IsEmpty();
        }
        if (handler->IsConsole())
            return false;
        Player* target = handler->getSelectedPlayer();
        if (!target)
            target = handler->GetSession()->GetPlayer();
        guid = target->GetGUID();
        outName = target->GetName();
        return true;
    }

    // .treasure reset <id|all> [character]
    static bool HandleReset(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> w = Words(args);
        if (w.empty())
        {
            handler->SendSysMessage("Usage: .treasure reset <id|all> [character]");
            return true;
        }
        ObjectGuid guid;
        std::string name;
        if (!Character(handler, w.size() > 1 ? w[1] : "", guid, name))
        {
            handler->SendSysMessage("Character not found.");
            return true;
        }
        bool all = w[0] == "all";
        uint32 spawnId = all ? 0 : uint32(atoi(w[0].c_str()));
        if (all)
            CharacterDatabase.PExecute("DELETE FROM character_treasure WHERE guid = %u", guid.GetCounter());
        else
            CharacterDatabase.PExecute("DELETE FROM character_treasure WHERE guid = %u AND spawn_id = %u", guid.GetCounter(), spawnId);
        Player* online = ObjectAccessor::FindPlayer(guid);
        if (online)
        {
            {
                std::unique_lock<std::shared_mutex> guard(lootedLock);
                auto it = looted.find(guid.GetCounter());
                if (it != looted.end())
                {
                    if (all)
                        it->second.clear();
                    else
                        it->second.erase(spawnId);
                }
            }
            RefreshNearby(online);
        }
        handler->PSendSysMessage("Reset %s for %s.", all ? "every treasure" : ("treasure " + std::to_string(spawnId)).c_str(), name.c_str());
        return true;
    }

    // .treasure looted [character]
    static bool HandleLooted(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> w = Words(args);
        ObjectGuid guid;
        std::string name;
        if (!Character(handler, w.empty() ? "" : w[0], guid, name))
        {
            handler->SendSysMessage("Character not found.");
            return true;
        }
        std::ostringstream ids;
        uint32 count = 0;
        if (QueryResult result = CharacterDatabase.PQuery("SELECT spawn_id FROM character_treasure WHERE guid = %u ORDER BY spawn_id", guid.GetCounter()))
            do
            {
                ids << (count++ ? ", " : "") << result->Fetch()[0].GetUInt32();
            } while (result->NextRow());
        handler->PSendSysMessage("%s has looted %u treasures%s%s", name.c_str(), count, count ? ": " : ".", ids.str().c_str());
        return true;
    }

    // .treasure explain [id]: the spot, its loot, and for every player on its map whether they looted it,
    // whether the server lets them see it and whether their client holds it.
    static bool HandleExplain(ChatHandler* handler, char const* args)
    {
        uint32 spawnId = args && *args ? uint32(atoi(args)) : 0;
        GameObjectData const* data = nullptr;
        if (spawnId)
            data = Find(handler, spawnId);
        else if (!handler->IsConsole())
        {
            Player* player = handler->GetSession()->GetPlayer();
            float best = 50.0f;
            for (auto const& t : AllTreasures(player->GetMapId()))
            {
                float d = player->GetDistance(t.second->posX, t.second->posY, t.second->posZ);
                if (d < best)
                {
                    best = d;
                    spawnId = t.first;
                    data = t.second;
                }
            }
            if (!data)
                handler->SendSysMessage("No treasure within 50 yd; give an id.");
        }
        else
            handler->SendSysMessage("Usage: .treasure explain <id>");
        if (!data)
            return true;

        GameObjectTemplate const* info = sObjectMgr->GetGameObjectTemplate(data->id);
        GameObjectTemplateAddon const* addon = sObjectMgr->GetGameObjectTemplateAddon(data->id);
        auto spot = spots.find(spawnId);
        handler->PSendSysMessage("Treasure %u: %s (entry %u, loot %u, %u-%u copper) map %u at %.1f %.1f %.1f",
            spawnId, Label(spawnId, data).c_str(), data->id, info->GetLootId(), addon ? addon->mingold : 0, addon ? addon->maxgold : 0,
            data->mapid, data->posX, data->posY, data->posZ);
        if (spot != spots.end() && !spot->second.Landmark.empty())
            handler->PSendSysMessage("  Landmark: %s", spot->second.Landmark.c_str());
        uint32 total = 0;
        if (QueryResult result = CharacterDatabase.PQuery("SELECT COUNT(*) FROM character_treasure WHERE spawn_id = %u", spawnId))
            total = uint32(result->Fetch()[0].GetUInt64());
        handler->PSendSysMessage("  Looted by %u characters in total.", total);

        Map* map = sMapMgr->FindBaseNonInstanceMap(data->mapid);
        GameObject* go = nullptr;
        if (map)
        {
            auto bounds = map->GetGameObjectBySpawnIdStore().equal_range(spawnId);
            if (bounds.first != bounds.second)
                go = bounds.first->second;
        }
        if (!go)
        {
            handler->SendSysMessage("  Not loaded (no player near it).");
            return true;
        }
        for (auto const& ref : map->GetPlayers())
        {
            Player* p = ref.GetSource();
            if (!p || p->GetDistance(go) > p->GetVisibilityRange())
                continue;
            bool lootedIt = HasLooted(p->GetGUID().GetCounter(), spawnId);
            bool canSee = p->CanSeeOrDetect(go, false, true);
            handler->PSendSysMessage("  %s: %s, %s, %s client (%.0f yd)", p->GetName().c_str(),
                lootedIt ? "looted" : "not looted", canSee ? "visible" : "hidden", p->HaveAtClient(go) ? "at" : "not at",
                p->GetDistance(go));
        }
        return true;
    }

    // .treasure validate <x> <y> <z> [map] [roadX roadY roadZ] [nudge]
    // In game, the map defaults to yours and the road point to where you stand.
    static bool HandleValidate(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> w = Words(args);
        bool nudge = !w.empty() && w.back() == "nudge";
        if (nudge)
            w.pop_back();
        std::vector<float> f;
        for (std::string const& word : w)
        {
            float value;
            if (!ParseFloat(word, value))
            {
                handler->SendSysMessage("Usage: .treasure validate <x> <y> <z> [map] [roadX roadY roadZ] [nudge]");
                return true;
            }
            f.push_back(value);
        }
        if (f.size() != 3 && f.size() != 4 && f.size() != 7)
        {
            handler->SendSysMessage("Usage: .treasure validate <x> <y> <z> [map] [roadX roadY roadZ] [nudge]");
            return true;
        }
        Player* player = handler->IsConsole() ? nullptr : handler->GetSession()->GetPlayer();
        uint32 mapId = f.size() >= 4 ? uint32(f[3]) : (player ? player->GetMapId() : 1);
        bool hasRoad = f.size() == 7 || (player && player->GetMapId() == mapId);
        float rx = f.size() == 7 ? f[4] : (player ? player->GetPositionX() : 0.0f);
        float ry = f.size() == 7 ? f[5] : (player ? player->GetPositionY() : 0.0f);
        float rz = f.size() == 7 ? f[6] : (player ? player->GetPositionZ() : 0.0f);
        Map* map = sMapMgr->CreateBaseMap(mapId);
        if (!map || map->Instanceable())
        {
            handler->PSendSysMessage("Map %u is not an open-world map.", mapId);
            return true;
        }
        Probe p = MeasureWithNudge(map, f[0], f[1], f[2], hasRoad, rx, ry, rz, 0, nudge);
        bool moved = std::fabs(p.X - f[0]) > 0.01f || std::fabs(p.Y - f[1]) > 0.01f;
        Report(handler, p, mapId, moved ? "Nudged to " : "Spot ");
        if (!hasRoad)
            handler->SendSysMessage("  (no road point: reachability not checked)");
        return true;
    }

    // .treasure check <id|all>: re-validates placed treasures against their stored road points.
    static bool HandleCheck(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> w = Words(args);
        if (w.empty())
        {
            handler->SendSysMessage("Usage: .treasure check <id|all>");
            return true;
        }
        std::vector<std::pair<uint32, GameObjectData const*>> list;
        if (w[0] == "all")
            list = AllTreasures();
        else if (GameObjectData const* data = Find(handler, uint32(atoi(w[0].c_str()))))
            list.emplace_back(uint32(atoi(w[0].c_str())), data);
        uint32 passed = 0;
        for (auto const& t : list)
        {
            Map* map = sMapMgr->CreateBaseMap(t.second->mapid);
            if (!map || map->Instanceable())
                continue;
            auto spot = spots.find(t.first);
            bool hasRoad = spot != spots.end() && spot->second.HasRoad;
            Probe p = Measure(map, t.second->posX, t.second->posY, t.second->posZ, hasRoad,
                hasRoad ? spot->second.RoadX : 0.0f, hasRoad ? spot->second.RoadY : 0.0f, hasRoad ? spot->second.RoadZ : 0.0f, t.first);
            passed += p.V.Passed() ? 1 : 0;
            Report(handler, p, t.second->mapid, (std::to_string(t.first) + " " + Label(t.first, t.second) + ": ").c_str());
        }
        handler->PSendSysMessage("%u of %u treasures pass.", passed, uint32(list.size()));
        return true;
    }

    // .treasure reload: re-reads treasure_spot and the config keys.
    static bool HandleReload(ChatHandler* handler, char const* /*args*/)
    {
        LoadConfig();
        LoadSpots();
        handler->PSendSysMessage("Treasures reloaded: %u spot rows.", uint32(spots.size()));
        return true;
    }
};

void AddSC_treasures()
{
    GameObjectHooks::RegisterHideFilter(&HideFilter);
    GameObjectHooks::RegisterSparkleFilter(&SparkleFilter);
    new go_treasure();
    new treasures_playerscript();
    new treasures_worldscript();
    new treasures_commandscript();
}
