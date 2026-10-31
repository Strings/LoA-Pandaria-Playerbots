/*
 * Heartseed state and data. See HeartseedMgr.h and README.md.
 */

#include "HeartseedMgr.h"
#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "GossipDef.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "QuestDef.h"
#include "Random.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Timer.h"
#include "WorldSession.h"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <sstream>

namespace
{
    // Set while the proc deals its damage, so that damage never rolls another proc.
    thread_local bool InProc = false;

    // Deals a proc (core or essence) one update after the hit that rolled it, outside the damage code.
    class HeartseedProcEvent : public BasicEvent
    {
    public:
        HeartseedProcEvent(Player* player, ObjectGuid victim, uint32 essenceId) : _player(player), _victim(victim), _essenceId(essenceId) { }

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            Unit* victim = ObjectAccessor::GetUnit(*_player, _victim);
            if (victim && victim->IsAlive() && _player->IsAlive() && _player->IsValidAttackTarget(victim))
                sHeartseedMgr->CastProc(_player, victim, _essenceId);
            return true;
        }

    private:
        Player* _player;
        ObjectGuid _victim;
        uint32 _essenceId;
    };

    // Asks about the replaced main hand one update after the swap, once both items sit where they end up.
    class HeartseedFeedEvent : public BasicEvent
    {
    public:
        HeartseedFeedEvent(Player* player, ObjectGuid oldWeapon) : _player(player), _oldWeapon(oldWeapon) { }

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            sHeartseedMgr->OfferReplacedFeed(_player, _oldWeapon);
            return true;
        }

    private:
        Player* _player;
        ObjectGuid _oldWeapon;
    };

    // Shows the granting quest a moment after login, once the client's UI is ready for a quest window.
    class HeartseedOfferEvent : public BasicEvent
    {
    public:
        explicit HeartseedOfferEvent(Player* player) : _player(player) { }

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            sHeartseedMgr->OfferGrantQuest(_player);
            return true;
        }

    private:
        Player* _player;
    };

    // Opens the reward window right after the quest is accepted, outside the accept handler.
    class HeartseedGrantRewardEvent : public BasicEvent
    {
    public:
        HeartseedGrantRewardEvent(Player* player, uint32 questId) : _player(player), _questId(questId) { }

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            Quest const* quest = sObjectMgr->GetQuestTemplate(_questId);
            if (!quest || sHeartseedMgr->HasHeartseed(_player))
                return true;
            if (_player->GetQuestStatus(_questId) == QUEST_STATUS_INCOMPLETE)
                _player->CompleteQuest(_questId);
            if (_player->GetQuestStatus(_questId) == QUEST_STATUS_COMPLETE)
                _player->PlayerTalkClass->SendQuestGiverOfferReward(quest, _player->GetGUID(), true);
            return true;
        }

    private:
        Player* _player;
        uint32 _questId;
    };

    std::string ToLower(std::string text)
    {
        std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return char(std::tolower(c)); });
        return text;
    }

    template<typename T>
    std::vector<T> ParseList(std::string const& text)
    {
        std::vector<T> values;
        std::istringstream stream(text);
        double value;
        while (stream >> value)
            values.push_back(T(value));
        return values;
    }
}

HeartseedMgr* HeartseedMgr::instance()
{
    static HeartseedMgr instance;
    return &instance;
}

void HeartseedMgr::LoadConfig()
{
    _enabled = sConfigMgr->GetBoolDefault("Heartseed.Enabled", true);
    _coreWeaponDamagePct = sConfigMgr->GetFloatDefault("Heartseed.Core.WeaponDamagePct", 5.0f);
    _procSpellId = sConfigMgr->GetIntDefault("Heartseed.Proc.SpellId", 16614);
    _procChance = sConfigMgr->GetFloatDefault("Heartseed.Proc.Chance", 10.0f);
    _procWeaponDamagePct = sConfigMgr->GetFloatDefault("Heartseed.Proc.WeaponDamagePct", 50.0f);
    _procCooldown = sConfigMgr->GetIntDefault("Heartseed.Proc.Cooldown", 3000);

    std::lock_guard<std::mutex> guard(_lock);
    _capPerLevel = std::max(0.0f, sConfigMgr->GetFloatDefault("Heartseed.Rank.CapPerLevel", 1.0f));
    _capPerAwakening = sConfigMgr->GetIntDefault("Heartseed.Rank.CapPerAwakening", 5);
    _awakeningFlags = ParseList<uint16>(sConfigMgr->GetStringDefault("Heartseed.Rank.AwakeningFlags", "1 2 3"));
    _rankGrowthBase = sConfigMgr->GetIntDefault("Heartseed.Rank.GrowthBase", 100);
    _rankGrowthPerRank = sConfigMgr->GetIntDefault("Heartseed.Rank.GrowthPerRank", 20);
    _feedOnReplace = sConfigMgr->GetBoolDefault("Heartseed.Feed.OnReplace", true);
    _feedGrowthBase = sConfigMgr->GetIntDefault("Heartseed.Feed.GrowthBase", 20);
    _feedGrowthPerItemLevel = sConfigMgr->GetFloatDefault("Heartseed.Feed.GrowthPerItemLevel", 2.0f);
    _feedQualityPct = ParseList<float>(sConfigMgr->GetStringDefault("Heartseed.Feed.QualityPct", "50 100 150 250 400 0 0 0"));
    _essenceProcCooldown = sConfigMgr->GetIntDefault("Heartseed.Essence.ProcCooldown", 3000);
    _essenceAutoSlot = sConfigMgr->GetBoolDefault("Heartseed.Essence.AutoSlot", true);
    _storyHooks = sConfigMgr->GetBoolDefault("Heartseed.Story.Hooks", true);
    _pandariaMaps = ParseList<uint32>(sConfigMgr->GetStringDefault("Heartseed.Story.PandariaMaps", "870"));
    std::vector<uint32> bosses = ParseList<uint32>(sConfigMgr->GetStringDefault("Heartseed.Story.WorldBosses",
        "60491 62346 69099 69161 71952 71953 71954 71955 72057 6109 12397 14887 14888 14889 14890 17711 18728"));
    _worldBossEntries = std::set<uint32>(bosses.begin(), bosses.end());
    _awakeningSound = sConfigMgr->GetIntDefault("Heartseed.Story.AwakeningSound", 0);
    _grantQuestId = sConfigMgr->GetIntDefault("Heartseed.Grant.QuestId", 90000);
    _grantOfferOnLogin = sConfigMgr->GetBoolDefault("Heartseed.Grant.OfferOnLogin", true);
    _grantOfferDelay = sConfigMgr->GetIntDefault("Heartseed.Grant.OfferDelay", 5000);
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

    std::map<uint32, Heartseed::TraitInfo> traits;
    if (QueryResult result = WorldDatabase.Query("SELECT `id`, `path`, `kind`, `tier`, `min_rank`, `spell_id`, `base_amount`, `amount_per_ilvl`, `name`, `description` FROM `heartseed_trait`"))
    {
        do
        {
            Field* fields = result->Fetch();
            Heartseed::TraitInfo trait;
            trait.Id = fields[0].GetUInt32();
            uint8 path = fields[1].GetUInt8();
            uint8 kind = fields[2].GetUInt8();
            trait.Tier = fields[3].GetUInt8();
            trait.MinRank = fields[4].GetUInt8();
            trait.SpellId = fields[5].GetUInt32();
            trait.BaseAmount = fields[6].GetInt32();
            trait.AmountPerItemLevel = fields[7].GetFloat();
            trait.Name = fields[8].GetString();
            trait.Description = fields[9].GetString();

            if (!trait.Id || path >= Heartseed::MaxPaths)
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_trait`: trait %u has id 0 or unknown path %u, skipped.", trait.Id, path);
                continue;
            }
            if (kind != uint8(Heartseed::TraitKind::Minor) && kind != uint8(Heartseed::TraitKind::Major))
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_trait`: trait %u has unknown kind %u (1 = minor, 2 = major), skipped.", trait.Id, kind);
                continue;
            }
            trait.TraitPath = Heartseed::Path(path);
            trait.Kind = Heartseed::TraitKind(kind);
            if (trait.Kind == Heartseed::TraitKind::Major && trait.Tier >= Heartseed::MaxMajorTiers)
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_trait`: major %u is in tier %u; tiers are 0 to %u. Skipped.", trait.Id, trait.Tier, Heartseed::MaxMajorTiers - 1);
                continue;
            }
            SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(trait.SpellId);
            if (!spellInfo || !spellInfo->IsPassive() || !spellInfo->Effects[EFFECT_0].IsAura())
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_trait`: trait %u uses spell %u, which is missing, not passive or has no aura in effect 0. Skipped.", trait.Id, trait.SpellId);
                continue;
            }
            traits[trait.Id] = std::move(trait);
        } while (result->NextRow());
    }

    std::map<uint32, Heartseed::EssenceInfo> essences;
    if (QueryResult result = WorldDatabase.Query("SELECT `id`, `name`, `proc_spell_id`, `proc_chance`, `weapon_damage_pct`, `description` FROM `heartseed_essence_template`"))
    {
        do
        {
            Field* fields = result->Fetch();
            Heartseed::EssenceInfo essence;
            essence.Id = fields[0].GetUInt32();
            essence.Name = fields[1].GetString();
            essence.ProcSpellId = fields[2].GetUInt32();
            essence.ProcChance = fields[3].GetFloat();
            essence.WeaponDamagePct = fields[4].GetFloat();
            essence.Description = fields[5].GetString();
            if (!essence.Id)
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_essence_template`: essence id 0 is reserved for the core proc, skipped.");
                continue;
            }
            if (essence.ProcSpellId && !sSpellMgr->GetSpellInfo(essence.ProcSpellId))
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_essence_template`: essence %u uses unknown spell %u; it is collected but never procs.", essence.Id, essence.ProcSpellId);
                essence.ProcSpellId = 0;
            }
            essences[essence.Id] = std::move(essence);
        } while (result->NextRow());
    }

    std::vector<Heartseed::EssenceSource> essenceSources;
    if (QueryResult result = WorldDatabase.Query("SELECT `essence_id`, `item_entry`, `name_pattern`, `chance` FROM `heartseed_essence_source`"))
    {
        do
        {
            Field* fields = result->Fetch();
            Heartseed::EssenceSource source;
            source.EssenceId = fields[0].GetUInt32();
            source.ItemEntry = fields[1].GetUInt32();
            source.NamePattern = ToLower(fields[2].GetString());
            source.Chance = fields[3].GetFloat();
            if (!essences.count(source.EssenceId))
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_essence_source`: essence %u does not exist, skipped.", source.EssenceId);
                continue;
            }
            if (!source.ItemEntry && source.NamePattern.empty())
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_essence_source`: a row for essence %u has neither an item entry nor a name pattern, skipped.", source.EssenceId);
                continue;
            }
            essenceSources.push_back(std::move(source));
        } while (result->NextRow());
    }

    std::set<uint32> twinkItems;
    if (QueryResult result = WorldDatabase.Query("SELECT `entry` FROM `twink_item`"))
    {
        do
            twinkItems.insert(result->Fetch()[0].GetUInt32());
        while (result->NextRow());
    }

    if (!sSpellMgr->GetSpellInfo(Heartseed::SPELL_CORE_WEAPON_BONUS))
        TC_LOG_ERROR("sql.sql", "Heartseed: core passive spell %u is missing from `spell_dbc`; no bonus weapon damage.", Heartseed::SPELL_CORE_WEAPON_BONUS);
    if (_procSpellId && !sSpellMgr->GetSpellInfo(_procSpellId))
        TC_LOG_ERROR("server.loading", "Heartseed.Proc.SpellId %u does not exist; the core proc stays off.", _procSpellId);

    {
        std::lock_guard<std::mutex> guard(_lock);
        _stages = std::move(stages);
        _stageMinLevels = std::move(minLevels);
        _glow = std::move(glow);
        _traits = std::move(traits);
        _essences = std::move(essences);
        _essenceSources = std::move(essenceSources);
        _twinkItems = std::move(twinkItems);
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u Heartseed stages, %u glows, %u traits, %u essences, %u essence sources and %u twink items in %u ms",
        uint32(_stages.size()), uint32(_glow.size()), uint32(_traits.size()), uint32(_essences.size()), uint32(_essenceSources.size()),
        uint32(_twinkItems.size()), GetMSTimeDiffToNow(oldMSTime));
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

    if (QueryResult result = CharacterDatabase.PQuery("SELECT CAST(COALESCE(SUM(`count`), 0) AS UNSIGNED) FROM `heartseed_fed_weapon` WHERE `guid` = %u", guid))
        state.FedWeapons = uint32(result->Fetch()[0].GetUInt64());

    if (Item* weapon = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
        state.MainHand = weapon->GetGUID();
    state.FirstNewItemGuid = sObjectMgr->GetGenerator<HighGuid::Item>().GetNextAfterMaxUsed();

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
        itr->second.FirstNewItemGuid = sObjectMgr->GetGenerator<HighGuid::Item>().GetNextAfterMaxUsed();
        if (Item* weapon = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND))
            itr->second.MainHand = weapon->GetGUID();
        itr->second.Dirty = true;
    }

    SavePlayer(player);
    Refresh(player);
    return true;
}

bool HeartseedMgr::Revoke(Player* player)
{
    uint32 guid = player->GetGUID().GetCounter();
    std::map<uint32, int32> applied;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(guid);
        if (itr == _states.end())
            return false;
        applied = std::move(itr->second.AppliedAuras);
        _states.erase(itr);
    }

    RemoveAuras(player, applied);

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM `heartseed_state` WHERE `guid` = %u", guid);
    trans->PAppend("DELETE FROM `heartseed_loadout` WHERE `guid` = %u", guid);
    trans->PAppend("DELETE FROM `heartseed_essence` WHERE `guid` = %u", guid);
    trans->PAppend("DELETE FROM `heartseed_story_flag` WHERE `guid` = %u", guid);
    trans->PAppend("DELETE FROM `heartseed_fed_weapon` WHERE `guid` = %u", guid);
    CharacterDatabase.CommitTransaction(trans);

    // The quest is the canonical marker, so revoking makes it available again.
    if (player->GetQuestStatus(_grantQuestId) != QUEST_STATUS_NONE)
        player->RemoveActiveQuest(_grantQuestId, true, true);
    player->RemoveRewardedQuest(_grantQuestId);

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
    Refresh(player);
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
    if (!ModifyState(player, [flag, set](Heartseed::State& state)
    {
        if (!set)
            state.StoryFlags.erase(flag);
        else if (!state.StoryFlags.count(flag))
            state.StoryFlags[flag] = uint32(time(nullptr));
    }))
        return false;

    // An awakening raises the cap, and banked Growth ranks the Heartseed up at once.
    if (set)
        ApplyRankUps(player);
    return true;
}

void HeartseedMgr::ScheduleGrantOffer(Player* player)
{
    if (_grantOfferOnLogin && CanOfferGrantQuest(player))
        player->m_Events.AddEvent(new HeartseedOfferEvent(player), player->m_Events.CalculateTime(_grantOfferDelay));
}

bool HeartseedMgr::CanOfferGrantQuest(Player* player) const
{
    // The quest's completion is the canonical "has a Heartseed"; a GM grant without it still counts as having one.
    return _enabled && !HasHeartseed(player) && sObjectMgr->GetQuestTemplate(_grantQuestId)
        && !player->GetQuestRewardStatus(_grantQuestId);
}

bool HeartseedMgr::OfferGrantQuest(Player* player)
{
    if (!player->IsInWorld() || !CanOfferGrantQuest(player))
        return false;

    Quest const* quest = sObjectMgr->GetQuestTemplate(_grantQuestId);
    switch (player->GetQuestStatus(_grantQuestId))
    {
        case QUEST_STATUS_NONE:
            if (!player->CanTakeQuest(quest, false))
                return false;
            // The player is its own quest giver; the quest's auto-take flag lets the accept through.
            player->PlayerTalkClass->SendQuestGiverQuestDetails(quest, player->GetGUID(), true);
            return true;
        case QUEST_STATUS_INCOMPLETE:
        case QUEST_STATUS_COMPLETE:
            // Accepted earlier but never turned in (logged out at the reward window).
            player->m_Events.AddEvent(new HeartseedGrantRewardEvent(player, _grantQuestId), player->m_Events.CalculateTime(0));
            return true;
        default:
            return false;
    }
}

void HeartseedMgr::OnQuestAdded(Player* player, Quest const* quest)
{
    if (quest->GetQuestId() == _grantQuestId)
        player->m_Events.AddEvent(new HeartseedGrantRewardEvent(player, _grantQuestId), player->m_Events.CalculateTime(0));
}

void HeartseedMgr::OnQuestRewarded(Player* player, Quest const* quest)
{
    // Any way of completing the quest grants it: this one, and later the Theramore intro scenario.
    if (quest->GetQuestId() == _grantQuestId && Grant(player))
        ChatHandler(player->GetSession()).SendSysMessage("A Heartseed has taken root in your weapon.");
}

bool HeartseedMgr::ReachStoryBeat(Player* player, uint16 flag)
{
    Heartseed::State state;
    if (!GetState(player, state) || state.StoryFlags.count(flag))
        return false;

    // Told before the flag is set, so the awakening reads before the rank-ups it causes.
    std::vector<uint16> awakeningFlags = GetAwakeningFlags();
    bool awakening = std::find(awakeningFlags.begin(), awakeningFlags.end(), flag) != awakeningFlags.end();
    ChatHandler handler(player->GetSession());
    if (awakening)
    {
        state.StoryFlags[flag] = 0;
        uint8 newCap = Heartseed::RankCap(player->GetLevel(), _capPerLevel, GetAwakenings(state), _capPerAwakening);
        handler.PSendSysMessage("Your Heartseed awakens (%s). It can grow to rank %u now.", GetStoryFlagName(flag), uint32(newCap));
        if (_awakeningSound)
            player->PlayDirectSound(_awakeningSound, player);
    }
    else
        handler.PSendSysMessage("Your Heartseed remembers this (%s).", GetStoryFlagName(flag));

    return SetStoryFlag(player, flag, true);
}

void HeartseedMgr::OnDungeonCompleted(Player* player)
{
    if (_storyHooks)
        ReachStoryBeat(player, Heartseed::STORY_FLAG_FIRST_DUNGEON);
}

void HeartseedMgr::OnMapChanged(Player* player)
{
    if (!_storyHooks)
        return;
    if (std::find(_pandariaMaps.begin(), _pandariaMaps.end(), player->GetMapId()) != _pandariaMaps.end())
        ReachStoryBeat(player, Heartseed::STORY_FLAG_REACHED_PANDARIA);
}

void HeartseedMgr::OnKillCredit(Player* player, Creature* killed)
{
    if (_storyHooks && _worldBossEntries.count(killed->GetEntry()))
        ReachStoryBeat(player, Heartseed::STORY_FLAG_FIRST_WORLD_BOSS);
}

std::vector<uint16> HeartseedMgr::GetAwakeningFlags() const
{
    std::lock_guard<std::mutex> guard(_lock);
    return _awakeningFlags;
}

uint32 HeartseedMgr::GetAwakenings(Heartseed::State const& state) const
{
    std::lock_guard<std::mutex> guard(_lock);
    uint32 awakenings = 0;
    for (uint16 flag : _awakeningFlags)
        awakenings += state.StoryFlags.count(flag) ? 1 : 0;
    return awakenings;
}

uint8 HeartseedMgr::GetRankCap(Player const* player) const
{
    Heartseed::State state;
    if (!GetState(player, state))
        return 1;
    return Heartseed::RankCap(player->GetLevel(), _capPerLevel, GetAwakenings(state), _capPerAwakening);
}

uint32 HeartseedMgr::GetGrowthForNextRank(uint8 rank) const
{
    return Heartseed::GrowthForNextRank(rank, _rankGrowthBase, _rankGrowthPerRank);
}

uint8 HeartseedMgr::AddGrowth(Player* player, uint32 amount)
{
    if (!amount || !HasHeartseed(player))
        return 0;

    uint8 cap = GetRankCap(player);
    uint8 oldRank = 0;
    uint8 newRank = 0;
    bool capNotice = false;
    ModifyState(player, [&](Heartseed::State& state)
    {
        oldRank = state.Rank;
        Heartseed::RankProgress progress = Heartseed::ApplyGrowth(state.Rank, Heartseed::AddCapped(state.Growth, amount), cap,
            _rankGrowthBase, _rankGrowthPerRank);
        state.Rank = progress.Rank;
        state.Growth = progress.Growth;
        newRank = state.Rank;

        // Growth is banked at the cap; say once per session that the Heartseed waits on the character.
        if (state.Rank >= cap && state.Growth >= GetGrowthForNextRank(state.Rank) && !state.CapNoticeSent)
        {
            state.CapNoticeSent = true;
            capNotice = true;
        }
    });

    if (newRank > oldRank)
        SendRankUp(player, oldRank, newRank);
    if (capNotice)
        ChatHandler(player->GetSession()).SendSysMessage("Your Heartseed is full. It keeps what it is fed, and grows again as you do.");
    return newRank > oldRank ? newRank - oldRank : 0;
}

uint8 HeartseedMgr::ApplyRankUps(Player* player)
{
    if (!HasHeartseed(player))
        return 0;

    uint8 cap = GetRankCap(player);
    uint8 oldRank = 0;
    uint8 newRank = 0;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(player->GetGUID().GetCounter());
        if (itr == _states.end())
            return 0;
        oldRank = itr->second.Rank;
        Heartseed::RankProgress progress = Heartseed::ApplyGrowth(itr->second.Rank, itr->second.Growth, cap, _rankGrowthBase, _rankGrowthPerRank);
        if (progress.Rank == oldRank)
            return 0;
        newRank = progress.Rank;
        itr->second.Rank = progress.Rank;
        itr->second.Growth = progress.Growth;
        itr->second.CapNoticeSent = false;
        itr->second.Dirty = true;
    }

    SavePlayer(player);
    Refresh(player);
    SendRankUp(player, oldRank, newRank);
    return newRank - oldRank;
}

void HeartseedMgr::SendRankUp(Player* player, uint8 oldRank, uint8 newRank)
{
    ChatHandler(player->GetSession()).PSendSysMessage("Your Heartseed grows: rank %u.", uint32(newRank));
    TC_LOG_DEBUG("scripts", "Heartseed: %s rank %u -> %u.", player->GetName().c_str(), uint32(oldRank), uint32(newRank));
}

float HeartseedMgr::GetFeedQualityPct(uint32 quality) const
{
    std::lock_guard<std::mutex> guard(_lock);
    return quality < _feedQualityPct.size() ? _feedQualityPct[quality] : 0.0f;
}

bool HeartseedMgr::IsTwinkItem(uint32 entry) const
{
    std::lock_guard<std::mutex> guard(_lock);
    return _twinkItems.count(entry) != 0;
}

uint32 HeartseedMgr::GetTwinkItemCount() const
{
    std::lock_guard<std::mutex> guard(_lock);
    return uint32(_twinkItems.size());
}

void HeartseedMgr::RecordAppearance(Player* player, ItemTemplate const* proto, Heartseed::AppearanceSource source)
{
    // Keeps the first way each appearance was unlocked.
    CharacterDatabase.PExecute("INSERT IGNORE INTO `character_weapon_appearance` (`guid`, `item_entry`, `display_id`, `source`, `unlock_time`) "
        "VALUES (%u, %u, %u, %u, %u)", player->GetGUID().GetCounter(), proto->ItemId, proto->DisplayInfoID, uint32(source), uint32(time(nullptr)));
}

Heartseed::FeedResult HeartseedMgr::CanFeed(Player const* player, Item const* item, Heartseed::FeedSource source) const
{
    if (!HasHeartseed(player))
        return Heartseed::FeedResult::NoHeartseed;
    if (!_enabled)
        return Heartseed::FeedResult::Disabled;

    ItemTemplate const* proto = item->GetTemplate();
    if (proto->Class != ITEM_CLASS_WEAPON || proto->SubClass == ITEM_SUBCLASS_WEAPON_FISHING_POLE || proto->SubClass == ITEM_SUBCLASS_WEAPON_MISCELLANEOUS)
        return Heartseed::FeedResult::NotWeapon;
    if (proto->Bonding == BIND_QUEST || proto->StartQuest || GetFeedQualityPct(proto->Quality) <= 0.0f)
        return Heartseed::FeedResult::Excluded;
    // A refundable weapon still buys back its price, which would make vendors a Growth shop.
    if (item->HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_REFUNDABLE))
        return Heartseed::FeedResult::Refundable;
    if (item->IsEquipped())
        return Heartseed::FeedResult::Equipped;
    // Twink gear is kept on purpose; only deleting it (or a GM) feeds it.
    if (source == Heartseed::FeedSource::Replace && IsTwinkItem(proto->ItemId))
        return Heartseed::FeedResult::Twink;
    return Heartseed::FeedResult::Ok;
}

void HeartseedMgr::FeedPreview(ItemTemplate const* proto, uint32 itemLevel, Heartseed::FeedOutcome& out) const
{
    out.Growth = Heartseed::FeedGrowth(itemLevel, _feedGrowthBase, _feedGrowthPerItemLevel, GetFeedQualityPct(proto->Quality));
    out.EssenceChances.clear();

    std::string name = ToLower(proto->Name1);
    std::lock_guard<std::mutex> guard(_lock);
    // One roll per essence: several matching rows give the best chance among them, not several rolls.
    for (Heartseed::EssenceSource const& source : _essenceSources)
    {
        bool match = source.ItemEntry ? source.ItemEntry == proto->ItemId : name.find(source.NamePattern) != std::string::npos;
        if (!match || source.Chance <= 0.0f)
            continue;
        auto itr = std::find_if(out.EssenceChances.begin(), out.EssenceChances.end(),
            [&source](std::pair<uint32, float> const& chance) { return chance.first == source.EssenceId; });
        if (itr == out.EssenceChances.end())
            out.EssenceChances.emplace_back(source.EssenceId, std::min(source.Chance, 100.0f));
        else
            itr->second = std::max(itr->second, std::min(source.Chance, 100.0f));
    }
}

Heartseed::FeedResult HeartseedMgr::Feed(Player* player, Item* item, Heartseed::FeedOutcome& out, Heartseed::FeedSource source)
{
    Heartseed::FeedResult result = CanFeed(player, item, source);
    if (result != Heartseed::FeedResult::Ok)
        return result;

    ItemTemplate const* proto = item->GetTemplate();
    uint32 entry = proto->ItemId;
    std::string itemName = proto->Name1;
    FeedPreview(proto, item->GetItemLevel(), out);

    out.Essences.clear();
    for (auto const& [essenceId, chance] : out.EssenceChances)
        if (roll_chance_f(chance))
            out.Essences.push_back(essenceId);

    Heartseed::State before;
    GetState(player, before);
    out.OldRank = before.Rank;

    RecordAppearance(player, proto, Heartseed::APPEARANCE_FED);
    // Consumed first, so nothing below can leave the weapon behind after it has paid out. A deleted item is
    // destroyed by the delete handler right after this returns.
    if (source != Heartseed::FeedSource::Delete)
        player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);

    // Every weapon fed is kept by entry: consumed weapons become appearances later (DESIGN.md, Leveling).
    CharacterDatabase.PExecute("INSERT INTO `heartseed_fed_weapon` (`guid`, `item_entry`, `count`, `first_time`) VALUES (%u, %u, 1, %u) "
        "ON DUPLICATE KEY UPDATE `count` = `count` + 1", player->GetGUID().GetCounter(), entry, uint32(time(nullptr)));

    bool firstFeed = before.FedWeapons == 0;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(player->GetGUID().GetCounter());
        if (itr != _states.end())
            ++itr->second.FedWeapons;
    }

    ChatHandler handler(player->GetSession());
    handler.PSendSysMessage("Your Heartseed consumes %s: +%u Growth.", itemName.c_str(), out.Growth);
    if (firstFeed)
        handler.SendSysMessage("The Heartseed feeds on the weapons you outgrow. When a better weapon replaces your main hand, it asks to eat "
            "the old one and grows from it; essences of a weapon's origin may stay behind in it.");

    for (uint32 essenceId : out.Essences)
    {
        AddEssence(player, essenceId, 1);
        Heartseed::EssenceInfo essence;
        if (GetEssence(essenceId, essence))
            handler.PSendSysMessage("Something of its origin stays behind: %s.", essence.Name.c_str());
    }

    AddGrowth(player, out.Growth);
    Heartseed::State after;
    GetState(player, after);
    out.NewRank = after.Rank;
    return Heartseed::FeedResult::Ok;
}

void HeartseedMgr::OnMainHandChanged(Player* player, Item* newWeapon)
{
    // An emptied slot keeps the last weapon: a swap may empty the slot before the new weapon lands in it.
    if (!newWeapon || newWeapon->GetTemplate()->Class != ITEM_CLASS_WEAPON)
        return;

    ObjectGuid newGuid = newWeapon->GetGUID();
    ObjectGuid oldGuid;
    bool newSinceLogin = false;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(player->GetGUID().GetCounter());
        if (itr == _states.end())
            return;
        oldGuid = itr->second.MainHand;
        itr->second.MainHand = newGuid;
        newSinceLogin = newGuid.GetCounter() >= itr->second.FirstNewItemGuid;
    }

    // First twink weapon in the main hand: say once that twink gear never feeds the Heartseed on its own.
    if (_enabled && IsTwinkItem(newWeapon->GetEntry()))
    {
        Heartseed::State state;
        if (GetState(player, state) && !state.StoryFlags.count(Heartseed::HINT_TWINK_WEAPON))
        {
            SetStoryFlag(player, Heartseed::HINT_TWINK_WEAPON, true);
            player->GetSession()->SendNotification("Twink weapons never feed your Heartseed on their own.");
            ChatHandler(player->GetSession()).SendSysMessage("This is twink gear. Your Heartseed asks to eat the weapons you outgrow, but never "
                "twink weapons: they stay yours however you swap them. To feed one to the Heartseed, delete it from your bags.");
        }
    }

    // Only a weapon gained since login replaces anything. Swapping between weapons the character already
    // carried (a situational weapon, a second spec's) never asks about one of them.
    if (_enabled && _feedOnReplace && newSinceLogin && !oldGuid.IsEmpty() && oldGuid != newGuid)
        player->m_Events.AddEvent(new HeartseedFeedEvent(player, oldGuid), player->m_Events.CalculateTime(0));
}

void HeartseedMgr::OfferReplacedFeed(Player* player, ObjectGuid oldWeapon)
{
    Item* weapon = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
    Item* replaced = player->GetItemByGuid(oldWeapon);
    // Only an upgrade replaces: swapping to an equal or weaker weapon (a situational one) keeps both. The old
    // weapon must be in the bags, not sold, traded, banked or moved to the off hand.
    if (!weapon || !replaced || replaced == weapon || replaced->IsEquipped() || !player->IsInventoryPos(replaced->GetPos()))
        return;
    if (weapon->GetItemLevel() <= replaced->GetItemLevel())
        return;
    if (CanFeed(player, replaced, Heartseed::FeedSource::Replace) != Heartseed::FeedResult::Ok)
        return;

    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(player->GetGUID().GetCounter());
        if (itr == _states.end() || itr->second.DeclinedFeeds.count(oldWeapon))
            return;
        itr->second.PendingFeed = oldWeapon;
    }

    Heartseed::FeedOutcome outcome;
    FeedPreview(replaced->GetTemplate(), replaced->GetItemLevel(), outcome);
    std::string name = replaced->GetTemplate()->Name1;

    // A gossip menu from the player itself: the 5.4.8 client's only server-driven dialog with a confirm box.
    player->PlayerTalkClass->ClearMenus();
    GossipMenu& menu = player->PlayerTalkClass->GetGossipMenu();
    menu.SetMenuId(Heartseed::GOSSIP_MENU_FEED);
    menu.AddMenuItem(-1, GOSSIP_ICON_CHAT, "Feed " + name + " to the Heartseed (+" + std::to_string(outcome.Growth) + " Growth).",
        0, Heartseed::FEED_ACTION_FEED, "Your Heartseed will consume " + name + ". It cannot be undone.", 0);
    menu.AddMenuItem(-1, GOSSIP_ICON_CHAT, "Keep " + name + ".", 0, Heartseed::FEED_ACTION_KEEP, "", 0);
    player->PlayerTalkClass->SendGossipMenu(Heartseed::NPC_TEXT_FEED, player->GetGUID());
}

void HeartseedMgr::OnFeedGossipSelect(Player* player, uint32 action)
{
    player->PlayerTalkClass->SendCloseGossip();

    ObjectGuid pending;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(player->GetGUID().GetCounter());
        if (itr == _states.end())
            return;
        pending = itr->second.PendingFeed;
        itr->second.PendingFeed.Clear();
        if (action == Heartseed::FEED_ACTION_KEEP && !pending.IsEmpty())
            itr->second.DeclinedFeeds.insert(pending);
    }

    Item* item = pending.IsEmpty() ? nullptr : player->GetItemByGuid(pending);
    if (action != Heartseed::FEED_ACTION_FEED || !item)
        return;

    // Checked again: the weapon may have been moved, sold or equipped while the prompt was open.
    if (!player->IsInventoryPos(item->GetPos()))
        return;
    Heartseed::FeedOutcome outcome;
    Feed(player, item, outcome, Heartseed::FeedSource::Replace);
}

void HeartseedMgr::OnItemDestroyedByPlayer(Player* player, Item* item)
{
    ItemTemplate const* proto = item->GetTemplate();
    if (proto->Class != ITEM_CLASS_WEAPON)
        return;

    // A twink weapon is fed when the player deletes it; any other deleted weapon only unlocks its appearance.
    if (IsTwinkItem(proto->ItemId) && !item->IsEquipped())
    {
        Heartseed::FeedOutcome outcome;
        if (Feed(player, item, outcome, Heartseed::FeedSource::Delete) == Heartseed::FeedResult::Ok)
            return;
    }
    RecordAppearance(player, proto, Heartseed::APPEARANCE_DELETED);
}

Heartseed::EssenceResult HeartseedMgr::AddEssence(Player* player, uint32 essenceId, uint32 count)
{
    if (!HasHeartseed(player))
        return Heartseed::EssenceResult::NoHeartseed;
    Heartseed::EssenceInfo essence;
    if (!GetEssence(essenceId, essence))
        return Heartseed::EssenceResult::UnknownEssence;

    uint8 level = player->GetLevel();
    bool autoSlot = _essenceAutoSlot;
    ModifyState(player, [essenceId, count, level, autoSlot](Heartseed::State& state)
    {
        auto [itr, inserted] = state.Essences.try_emplace(essenceId, 0, 0);
        itr->second.first = Heartseed::AddCapped(itr->second.first, count);
        if (inserted && autoSlot)
        {
            std::vector<uint8> taken;
            for (auto const& [id, held] : state.Essences)
                if (held.second)
                    taken.push_back(held.second);
            itr->second.second = Heartseed::FreeEssenceSlot(taken, level);
        }
    });
    return Heartseed::EssenceResult::Ok;
}

Heartseed::EssenceResult HeartseedMgr::RemoveEssence(Player* player, uint32 essenceId)
{
    Heartseed::State current;
    if (!GetState(player, current))
        return Heartseed::EssenceResult::NoHeartseed;
    if (!current.Essences.count(essenceId))
        return Heartseed::EssenceResult::NotCollected;

    ModifyState(player, [essenceId](Heartseed::State& state) { state.Essences.erase(essenceId); });
    return Heartseed::EssenceResult::Ok;
}

Heartseed::EssenceResult HeartseedMgr::SetEssenceSlot(Player* player, uint32 essenceId, uint8 slot, bool force)
{
    Heartseed::State current;
    if (!GetState(player, current))
        return Heartseed::EssenceResult::NoHeartseed;
    if (!current.Essences.count(essenceId))
        return Heartseed::EssenceResult::NotCollected;
    if (slot > Heartseed::MaxEssenceSlots)
        return Heartseed::EssenceResult::WrongSlot;
    if (slot && !force && slot > Heartseed::EssenceSlotsForLevel(player->GetLevel()))
        return Heartseed::EssenceResult::SlotLocked;

    ModifyState(player, [essenceId, slot](Heartseed::State& state)
    {
        if (slot)
            for (auto& [id, held] : state.Essences)
                if (held.second == slot)
                    held.second = 0;
        state.Essences[essenceId].second = slot;
    });
    return Heartseed::EssenceResult::Ok;
}

std::vector<Heartseed::EssenceInfo> HeartseedMgr::GetEssences() const
{
    std::lock_guard<std::mutex> guard(_lock);
    std::vector<Heartseed::EssenceInfo> essences;
    for (auto const& [id, essence] : _essences)
        essences.push_back(essence);
    return essences;
}

bool HeartseedMgr::GetEssence(uint32 essenceId, Heartseed::EssenceInfo& out) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _essences.find(essenceId);
    if (itr == _essences.end())
        return false;
    out = itr->second;
    return true;
}

std::vector<Heartseed::EssenceSource> HeartseedMgr::GetEssenceSources() const
{
    std::lock_guard<std::mutex> guard(_lock);
    return _essenceSources;
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
        { Heartseed::HINT_TWINK_WEAPON,           "hint_twink_weapon" },
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

Heartseed::Path HeartseedMgr::GetDominantPath(Player const* player) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _states.find(player->GetGUID().GetCounter());
    if (itr == _states.end())
        return Heartseed::Path::None;
    return Heartseed::DominantPath(MajorsPerPath(itr->second, uint16(player->GetTalentSpecialization())));
}

std::array<uint8, Heartseed::MaxPaths> HeartseedMgr::MajorsPerPath(Heartseed::State const& state, uint16 specId) const
{
    std::array<uint8, Heartseed::MaxPaths> majors = { };
    auto loadout = state.Loadouts.find(specId);
    if (loadout == state.Loadouts.end())
        return majors;

    for (uint32 traitId : loadout->second)
    {
        auto trait = _traits.find(traitId);
        if (trait != _traits.end())
            ++majors[uint8(trait->second.TraitPath)];
    }
    return majors;
}

std::vector<Heartseed::TraitInfo> HeartseedMgr::GetTraits() const
{
    std::lock_guard<std::mutex> guard(_lock);
    std::vector<Heartseed::TraitInfo> traits;
    traits.reserve(_traits.size());
    for (auto const& [id, trait] : _traits)
        traits.push_back(trait);
    return traits;
}

bool HeartseedMgr::GetTrait(uint32 traitId, Heartseed::TraitInfo& out) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _traits.find(traitId);
    if (itr == _traits.end())
        return false;
    out = itr->second;
    return true;
}

Heartseed::ChoiceResult HeartseedMgr::ChooseMajor(Player* player, uint8 tier, uint32 traitId, bool force)
{
    if (!HasHeartseed(player))
        return Heartseed::ChoiceResult::NoHeartseed;
    if (tier >= Heartseed::MaxMajorTiers)
        return Heartseed::ChoiceResult::WrongTier;

    if (traitId)
    {
        Heartseed::TraitInfo trait;
        if (!GetTrait(traitId, trait))
            return Heartseed::ChoiceResult::UnknownTrait;
        if (trait.Kind != Heartseed::TraitKind::Major)
            return Heartseed::ChoiceResult::NotMajor;
        if (trait.Tier != tier)
            return Heartseed::ChoiceResult::WrongTier;
        if (!force && !Heartseed::MajorTierOpen(tier, player->GetLevel()))
            return Heartseed::ChoiceResult::TierLocked;
    }

    uint16 specId = uint16(player->GetTalentSpecialization());
    if (!ModifyState(player, [specId, tier, traitId](Heartseed::State& state) { state.Loadouts[specId][tier] = traitId; }))
        return Heartseed::ChoiceResult::NoHeartseed;
    return Heartseed::ChoiceResult::Ok;
}

void HeartseedMgr::Refresh(Player* player)
{
    RefreshTraits(player);
    RefreshGlow(player);
}

void HeartseedMgr::RefreshTraits(Player* player)
{
    Item* weapon = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
    if (weapon && (weapon->IsBroken() || weapon->GetTemplate()->Class != ITEM_CLASS_WEAPON))
        weapon = nullptr;

    uint32 itemLevel = weapon ? weapon->GetItemLevel() : 0;
    int32 weaponBonus = 0;
    if (weapon)
    {
        float minDamage = 0.0f;
        float maxDamage = 0.0f;
        player->GetItemDamage(minDamage, maxDamage, weapon);
        weaponBonus = int32(std::lround(Heartseed::CoreWeaponBonus(minDamage, maxDamage, _coreWeaponDamagePct)));
    }

    uint16 specId = uint16(player->GetTalentSpecialization());
    std::map<uint32, int32> desired;
    std::map<uint32, int32> previous;
    bool copiedLoadout = false;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(player->GetGUID().GetCounter());
        if (itr == _states.end())
            return;
        Heartseed::State& state = itr->second;

        // A specialization without a saved loadout starts from the one the character just left, so choosing a
        // spec after the first major (both happen at 10) or adding a second spec keeps the majors picked.
        if (!state.Loadouts.count(specId) && state.HasLastSpec && state.LastSpecId != specId)
        {
            auto from = state.Loadouts.find(state.LastSpecId);
            if (from != state.Loadouts.end())
            {
                state.Loadouts[specId] = from->second;
                state.Dirty = true;
                copiedLoadout = true;
            }
        }
        state.LastSpecId = specId;
        state.HasLastSpec = true;

        if (_enabled)
        {
            std::array<uint8, Heartseed::MaxPaths> majors = MajorsPerPath(state, specId);
            auto loadout = state.Loadouts.find(specId);
            for (auto const& [traitId, trait] : _traits)
            {
                bool active = trait.Kind == Heartseed::TraitKind::Minor
                    ? Heartseed::MinorActive(trait.TraitPath, trait.MinRank, state.Rank, majors)
                    : loadout != state.Loadouts.end() && loadout->second[trait.Tier] == traitId;
                if (active)
                    desired[trait.SpellId] += Heartseed::TraitAmount(trait.BaseAmount, trait.AmountPerItemLevel, itemLevel);
            }
            if (weaponBonus > 0)
                desired[Heartseed::SPELL_CORE_WEAPON_BONUS] += weaponBonus;
        }

        previous = std::move(state.AppliedAuras);
        state.AppliedAuras = desired;
    }

    // Aura changes happen outside the lock: removing or changing an aura can run other scripts.
    for (auto const& [spellId, amount] : previous)
        if (!desired.count(spellId))
            player->RemoveAurasDueToSpell(spellId);

    for (auto const& [spellId, amount] : desired)
    {
        Aura* aura = player->GetAura(spellId, player->GetGUID());
        if (!aura)
            aura = player->AddAura(spellId, player);
        if (!aura)
            continue;
        if (AuraEffect* effect = aura->GetEffect(EFFECT_0))
            if (effect->GetAmount() != amount)
                effect->ChangeAmount(amount);
    }

    if (copiedLoadout)
        SavePlayer(player);
}

void HeartseedMgr::RemoveAuras(Player* player, std::map<uint32, int32> const& auras)
{
    for (auto const& [spellId, amount] : auras)
        player->RemoveAurasDueToSpell(spellId);
}

void HeartseedMgr::OnDamageDealt(Player* player, Unit* victim)
{
    if (!_enabled || victim == player || InProc)
        return;

    uint32 now = getMSTime();
    uint8 openSlots = Heartseed::EssenceSlotsForLevel(player->GetLevel());
    std::vector<uint32> procs;                          // 0 = core proc, else essence id
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(player->GetGUID().GetCounter());
        if (itr == _states.end())
            return;
        Heartseed::State& state = itr->second;

        if (_procSpellId && _procChance > 0.0f && (!state.HasProcced || getMSTimeDiff(state.LastProcTime, now) >= _procCooldown)
            && roll_chance_f(_procChance))
        {
            state.LastProcTime = now;
            state.HasProcced = true;
            procs.push_back(0);
        }

        // Each essence in an open slot rolls on its own, with its own cooldown.
        for (auto const& [essenceId, held] : state.Essences)
        {
            if (!held.second || held.second > openSlots)
                continue;
            auto essence = _essences.find(essenceId);
            if (essence == _essences.end() || !essence->second.ProcSpellId || essence->second.ProcChance <= 0.0f)
                continue;
            auto last = state.EssenceLastProc.find(essenceId);
            if (last != state.EssenceLastProc.end() && getMSTimeDiff(last->second, now) < _essenceProcCooldown)
                continue;
            if (!roll_chance_f(essence->second.ProcChance))
                continue;
            state.EssenceLastProc[essenceId] = now;
            procs.push_back(essenceId);
        }
    }

    for (uint32 essenceId : procs)
        player->m_Events.AddEvent(new HeartseedProcEvent(player, victim->GetGUID(), essenceId), player->m_Events.CalculateTime(0));
}

uint32 HeartseedMgr::GetProcDamage(Player* player, float weaponDamagePct) const
{
    Item* weapon = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
    WeaponAttackType attackType = weapon && weapon->GetTemplate()->IsRangedInventoryType() ? RANGED_ATTACK : BASE_ATTACK;
    return Heartseed::CoreProcDamage(player->GetWeaponDamageRange(attackType, MINDAMAGE), player->GetWeaponDamageRange(attackType, MAXDAMAGE),
        weaponDamagePct);
}

void HeartseedMgr::CastProc(Player* player, Unit* victim, uint32 essenceId)
{
    uint32 spellId = _procSpellId;
    float damagePct = _procWeaponDamagePct;
    if (essenceId)
    {
        Heartseed::EssenceInfo essence;
        if (!GetEssence(essenceId, essence))
            return;
        spellId = essence.ProcSpellId;
        damagePct = essence.WeaponDamagePct;
    }

    if (!spellId || !sSpellMgr->GetSpellInfo(spellId))
        return;
    int32 damage = int32(GetProcDamage(player, damagePct));
    if (damage <= 0)
        return;

    // Dealt directly rather than cast: a cast checks range and facing even when triggered, which would drop
    // procs from ranged hits. The spell supplies the name, school and bonuses shown in the combat log.
    InProc = true;
    player->SpellNonMeleeDamageLog(victim, spellId, uint32(damage));
    InProc = false;

    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _states.find(player->GetGUID().GetCounter());
    if (itr != _states.end())
        ++(essenceId ? itr->second.EssenceProcCount : itr->second.ProcCount);
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
