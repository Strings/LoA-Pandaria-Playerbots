/*
 * Heartseed state and data. See HeartseedMgr.h and README.md.
 */

#include "HeartseedMgr.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Log.h"
#include "Player.h"
#include "Timer.h"
#include "WorldSession.h"

#include <ctime>

HeartseedMgr* HeartseedMgr::instance()
{
    static HeartseedMgr instance;
    return &instance;
}

void HeartseedMgr::LoadConfig()
{
    _enabled = sConfigMgr->GetBoolDefault("Heartseed.Enabled", true);
}

void HeartseedMgr::LoadData()
{
    uint32 oldMSTime = getMSTime();

    std::vector<Heartseed::StageInfo> stages;
    std::vector<uint8> minLevels;
    if (QueryResult result = WorldDatabase.Query("SELECT `stage`, `min_level`, `name` FROM `heartseed_stage` ORDER BY `stage`"))
    {
        do
        {
            Field* fields = result->Fetch();
            uint8 stage = fields[0].GetUInt8();
            if (stage != stages.size())
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_stage`: stages must be numbered 0, 1, 2, ... without gaps; stopped at stage %u.", stage);
                break;
            }

            Heartseed::StageInfo info;
            info.MinLevel = fields[1].GetUInt8();
            info.Name = fields[2].GetString();
            if (!minLevels.empty() && info.MinLevel < minLevels.back())
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_stage`: stage %u starts below the stage before it; stopped there.", stage);
                break;
            }

            minLevels.push_back(info.MinLevel);
            stages.push_back(std::move(info));
        } while (result->NextRow());
    }

    std::map<std::pair<uint8, uint8>, uint16> glow;
    if (QueryResult result = WorldDatabase.Query("SELECT `stage`, `path`, `enchant_id` FROM `heartseed_glow`"))
    {
        do
        {
            Field* fields = result->Fetch();
            uint8 stage = fields[0].GetUInt8();
            uint8 path = fields[1].GetUInt8();
            uint16 enchantId = fields[2].GetUInt16();
            if (stage >= stages.size() || path >= Heartseed::MaxPaths)
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_glow`: stage %u path %u does not exist, skipped.", stage, path);
                continue;
            }
            if (enchantId && !sSpellItemEnchantmentStore.LookupEntry(enchantId))
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_glow`: stage %u path %u uses unknown enchant %u, skipped.", stage, path, enchantId);
                continue;
            }
            glow[{ stage, path }] = enchantId;
        } while (result->NextRow());
    }

    {
        std::lock_guard<std::mutex> guard(_lock);
        _stages = std::move(stages);
        _stageMinLevels = std::move(minLevels);
        _glow = std::move(glow);
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u Heartseed stages and %u glows in %u ms", uint32(_stages.size()), uint32(_glow.size()), GetMSTimeDiffToNow(oldMSTime));
}

void HeartseedMgr::LoadPlayer(Player* player)
{
    uint32 guid = player->GetGUID().GetCounter();

    QueryResult stateResult = CharacterDatabase.PQuery("SELECT `rank`, `growth`, `granted_time` FROM `heartseed_state` WHERE `guid` = %u", guid);
    if (!stateResult)
    {
        UnloadPlayer(player);
        return;
    }

    Heartseed::State state;
    Field* fields = stateResult->Fetch();
    state.Rank = fields[0].GetUInt8();
    state.Growth = fields[1].GetUInt32();
    state.GrantedTime = fields[2].GetUInt32();

    if (QueryResult result = CharacterDatabase.PQuery("SELECT `spec_id`, `tier`, `trait_id` FROM `heartseed_loadout` WHERE `guid` = %u", guid))
    {
        do
        {
            fields = result->Fetch();
            uint8 tier = fields[1].GetUInt8();
            if (tier >= Heartseed::MaxMajorTiers)
                continue;
            // operator[] value-initializes a new loadout, so unchosen tiers read 0
            state.Loadouts[fields[0].GetUInt16()][tier] = fields[2].GetUInt32();
        } while (result->NextRow());
    }

    if (QueryResult result = CharacterDatabase.PQuery("SELECT `essence_id`, `count`, `active_slot` FROM `heartseed_essence` WHERE `guid` = %u", guid))
    {
        do
        {
            fields = result->Fetch();
            state.Essences[fields[0].GetUInt32()] = { fields[1].GetUInt32(), fields[2].GetUInt8() };
        } while (result->NextRow());
    }

    if (QueryResult result = CharacterDatabase.PQuery("SELECT `flag_id`, `set_time` FROM `heartseed_story_flag` WHERE `guid` = %u", guid))
    {
        do
        {
            fields = result->Fetch();
            state.StoryFlags[fields[0].GetUInt16()] = fields[1].GetUInt32();
        } while (result->NextRow());
    }

    std::lock_guard<std::mutex> guard(_lock);
    _states[guid] = std::move(state);
}

void HeartseedMgr::SavePlayer(Player* player)
{
    uint32 guid = player->GetGUID().GetCounter();
    Heartseed::State state;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(guid);
        if (itr == _states.end() || !itr->second.Dirty)
            return;
        itr->second.Dirty = false;
        state = itr->second;
    }

    // Loadouts, essences and flags are written as whole sets: they are small and change rarely.
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("REPLACE INTO `heartseed_state` (`guid`, `rank`, `growth`, `granted_time`) VALUES (%u, %u, %u, %u)",
        guid, uint32(state.Rank), state.Growth, state.GrantedTime);

    trans->PAppend("DELETE FROM `heartseed_loadout` WHERE `guid` = %u", guid);
    for (auto const& [specId, traits] : state.Loadouts)
        for (uint8 tier = 0; tier < Heartseed::MaxMajorTiers; ++tier)
            if (traits[tier])
                trans->PAppend("INSERT INTO `heartseed_loadout` (`guid`, `spec_id`, `tier`, `trait_id`) VALUES (%u, %u, %u, %u)",
                    guid, uint32(specId), uint32(tier), traits[tier]);

    trans->PAppend("DELETE FROM `heartseed_essence` WHERE `guid` = %u", guid);
    for (auto const& [essenceId, essence] : state.Essences)
        trans->PAppend("INSERT INTO `heartseed_essence` (`guid`, `essence_id`, `count`, `active_slot`) VALUES (%u, %u, %u, %u)",
            guid, essenceId, essence.first, uint32(essence.second));

    trans->PAppend("DELETE FROM `heartseed_story_flag` WHERE `guid` = %u", guid);
    for (auto const& [flagId, setTime] : state.StoryFlags)
        trans->PAppend("INSERT INTO `heartseed_story_flag` (`guid`, `flag_id`, `set_time`) VALUES (%u, %u, %u)",
            guid, uint32(flagId), setTime);

    CharacterDatabase.CommitTransaction(trans);
}

void HeartseedMgr::UnloadPlayer(Player* player)
{
    std::lock_guard<std::mutex> guard(_lock);
    _states.erase(player->GetGUID().GetCounter());
}

bool HeartseedMgr::HasHeartseed(Player const* player) const
{
    std::lock_guard<std::mutex> guard(_lock);
    return _states.count(player->GetGUID().GetCounter()) != 0;
}

bool HeartseedMgr::GetState(Player const* player, Heartseed::State& out) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _states.find(player->GetGUID().GetCounter());
    if (itr == _states.end())
        return false;
    out = itr->second;
    return true;
}

bool HeartseedMgr::Grant(Player* player)
{
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto [itr, inserted] = _states.try_emplace(player->GetGUID().GetCounter());
        if (!inserted)
            return false;
        itr->second.GrantedTime = uint32(time(nullptr));
        itr->second.Dirty = true;
    }

    SavePlayer(player);
    RefreshGlow(player);
    return true;
}

bool HeartseedMgr::Revoke(Player* player)
{
    uint32 guid = player->GetGUID().GetCounter();
    {
        std::lock_guard<std::mutex> guard(_lock);
        if (!_states.erase(guid))
            return false;
    }

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM `heartseed_state` WHERE `guid` = %u", guid);
    trans->PAppend("DELETE FROM `heartseed_loadout` WHERE `guid` = %u", guid);
    trans->PAppend("DELETE FROM `heartseed_essence` WHERE `guid` = %u", guid);
    trans->PAppend("DELETE FROM `heartseed_story_flag` WHERE `guid` = %u", guid);
    CharacterDatabase.CommitTransaction(trans);

    RefreshGlow(player);
    return true;
}

bool HeartseedMgr::ModifyState(Player* player, std::function<void(Heartseed::State&)> const& change)
{
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(player->GetGUID().GetCounter());
        if (itr == _states.end())
            return false;
        change(itr->second);
        itr->second.Dirty = true;
    }

    // Saved now rather than with the character, so a crash or a GM test never loses the change.
    SavePlayer(player);
    RefreshGlow(player);
    return true;
}

bool HeartseedMgr::SetRank(Player* player, uint8 rank)
{
    return ModifyState(player, [rank](Heartseed::State& state) { state.Rank = rank; });
}

bool HeartseedMgr::SetGrowth(Player* player, uint32 growth)
{
    return ModifyState(player, [growth](Heartseed::State& state) { state.Growth = growth; });
}

bool HeartseedMgr::SetStoryFlag(Player* player, uint16 flag, bool set)
{
    return ModifyState(player, [flag, set](Heartseed::State& state)
    {
        if (!set)
            state.StoryFlags.erase(flag);
        else if (!state.StoryFlags.count(flag))
            state.StoryFlags[flag] = uint32(time(nullptr));
    });
}

namespace
{
    struct StoryFlagName
    {
        uint16 Flag;
        char const* Name;
    };

    StoryFlagName const StoryFlagNames[] =
    {
        { Heartseed::STORY_FLAG_FIRST_DUNGEON,    "first_dungeon" },
        { Heartseed::STORY_FLAG_REACHED_PANDARIA, "reached_pandaria" },
        { Heartseed::STORY_FLAG_FIRST_WORLD_BOSS, "first_world_boss" },
    };
}

uint16 HeartseedMgr::GetStoryFlagByName(std::string const& name)
{
    for (StoryFlagName const& entry : StoryFlagNames)
        if (name == entry.Name)
            return entry.Flag;
    return 0;
}

char const* HeartseedMgr::GetStoryFlagName(uint16 flag)
{
    for (StoryFlagName const& entry : StoryFlagNames)
        if (flag == entry.Flag)
            return entry.Name;
    return "unnamed";
}

uint8 HeartseedMgr::GetStage(Player const* player) const
{
    std::lock_guard<std::mutex> guard(_lock);
    return Heartseed::StageForLevel(_stageMinLevels, player->GetLevel());
}

std::string HeartseedMgr::GetStageName(uint8 stage) const
{
    std::lock_guard<std::mutex> guard(_lock);
    return stage < _stages.size() ? _stages[stage].Name : "Unknown";
}

Heartseed::Path HeartseedMgr::GetDominantPath(Player const* /*player*/) const
{
    // Majors map to paths through heartseed_trait (T-021). Until that table exists no path dominates.
    return Heartseed::Path::None;
}

uint16 HeartseedMgr::GetGlowEnchant(Player const* player) const
{
    if (!_enabled || !HasHeartseed(player))
        return 0;

    uint8 stage = GetStage(player);
    uint8 path = uint8(GetDominantPath(player));

    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _glow.find({ stage, path });
    if (itr == _glow.end() && path != uint8(Heartseed::Path::None))
        itr = _glow.find({ stage, uint8(Heartseed::Path::None) });
    return itr != _glow.end() ? itr->second : 0;
}

void HeartseedMgr::RefreshGlow(Player* player)
{
    // SetVisibleItemSlot asks the scripts for the glow (OnVisibleItemEnchant), so re-setting the slot is enough.
    player->SetVisibleItemSlot(EQUIPMENT_SLOT_MAINHAND, player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND));
}
