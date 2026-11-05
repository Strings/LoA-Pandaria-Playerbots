/*
 * Heartseed state and data. See HeartseedMgr.h and README.md.
 */

#include "HeartseedMgr.h"
#include "Chat.h"
#include "CinematicMgr.h"
#include "Config.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "GossipDef.h"
#include "Log.h"
#include "Map.h"
#include "ObjectMgr.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "QuestDef.h"
#include "Random.h"
#include "StringFormat.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Timer.h"
#include "WorldPacket.h"
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
            // A new character's intro cinematic hides any quest window; wait until it has played.
            if (_player->GetCinematicMgr()->IsOnCinematic())
                _player->m_Events.AddEvent(new HeartseedOfferEvent(_player), _player->m_Events.CalculateTime(sHeartseedMgr->GetGrantOfferDelay()));
            else
                sHeartseedMgr->OfferGrantQuest(_player);
            return true;
        }

    private:
        Player* _player;
    };

    // Opens the choice window a moment after login, after any granting-quest window.
    class HeartseedChoicePromptEvent : public BasicEvent
    {
    public:
        explicit HeartseedChoicePromptEvent(Player* player) : _player(player) { }

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            if (_player->GetCinematicMgr()->IsOnCinematic())
                _player->m_Events.AddEvent(new HeartseedChoicePromptEvent(_player), _player->m_Events.CalculateTime(sHeartseedMgr->GetChoicePromptDelay()));
            else if (sHeartseedMgr->HasUnchosenOpenTier(_player))
                sHeartseedMgr->SendChoiceMenu(_player);
            return true;
        }

    private:
        Player* _player;
    };

    char const* PathName(Heartseed::Path path)
    {
        switch (path)
        {
            case Heartseed::Path::Xuen:   return "Xuen";
            case Heartseed::Path::ChiJi:  return "Chi-Ji";
            case Heartseed::Path::Niuzao: return "Niuzao";
            case Heartseed::Path::Yulon:  return "Yu'lon";
            default:                      return "none";
        }
    }

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
    _xuenLightningVisual = sConfigMgr->GetIntDefault("Heartseed.Gift.XuenLightningVisual", 36);
    _xuenCatchUpDistance = std::max(0.0f, sConfigMgr->GetFloatDefault("Heartseed.Gift.XuenCatchUpDistance", 25.0f));
    _xuenFollowInterval = std::max(250, sConfigMgr->GetIntDefault("Heartseed.Gift.XuenFollowInterval", 1000));
    _plumeRadius = std::max(0.0f, sConfigMgr->GetFloatDefault("Heartseed.Gift.PlumeRadius", 30.0f));
    _plumeTargets = uint32(std::max(1, sConfigMgr->GetIntDefault("Heartseed.Gift.PlumeTargets", 3)));
    _retortRadius = std::max(0.0f, sConfigMgr->GetFloatDefault("Heartseed.Gift.RetortRadius", 8.0f));
    _breathRange = std::max(0.0f, sConfigMgr->GetFloatDefault("Heartseed.Gift.BreathRange", 12.0f));
    _procChance = sConfigMgr->GetFloatDefault("Heartseed.Proc.Chance", 10.0f);
    _procWeaponDamagePct = sConfigMgr->GetFloatDefault("Heartseed.Proc.WeaponDamagePct", 50.0f);
    _procCooldown = sConfigMgr->GetIntDefault("Heartseed.Proc.Cooldown", 3000);

    std::lock_guard<std::mutex> guard(_lock);
    _growthXpBase = sConfigMgr->GetIntDefault("Heartseed.Growth.XpBase", 300);
    _growthXpPerLevel = sConfigMgr->GetIntDefault("Heartseed.Growth.XpPerLevel", 50);
    _growthBattlegroundWin = sConfigMgr->GetIntDefault("Heartseed.Growth.BattlegroundWin", 100);
    _growthDungeonBoss = sConfigMgr->GetIntDefault("Heartseed.Growth.DungeonBoss", 50);
    _growthRaidBoss = sConfigMgr->GetIntDefault("Heartseed.Growth.RaidBoss", 100);
    _growthQuest = sConfigMgr->GetIntDefault("Heartseed.Growth.Quest", 15);
    _growthRare = sConfigMgr->GetIntDefault("Heartseed.Growth.Rare", 40);
    _growthLevelRange = uint8(std::clamp(sConfigMgr->GetIntDefault("Heartseed.Growth.LevelRange", 5), 0, 255));
    _growthTwinksEarn = sConfigMgr->GetBoolDefault("Heartseed.Growth.TwinksEarn", false);
    _growthResetAtShrineOnly = sConfigMgr->GetBoolDefault("Heartseed.Growth.ResetAtShrineOnly", false);
    _feedOnReplace = sConfigMgr->GetBoolDefault("Heartseed.Feed.OnReplace", true);
    std::vector<uint32> qualities = ParseList<uint32>(sConfigMgr->GetStringDefault("Heartseed.Feed.Qualities", "0 1 2 3 4"));
    _feedQualities = std::set<uint32>(qualities.begin(), qualities.end());
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
    _choicePromptOnLogin = sConfigMgr->GetBoolDefault("Heartseed.Choice.PromptOnLogin", true);
    _choicePromptDelay = sConfigMgr->GetIntDefault("Heartseed.Choice.PromptDelay", 6000);
    _choiceShrineOnly = sConfigMgr->GetBoolDefault("Heartseed.Choice.ChangeAtShrineOnly", false);

    Heartseed::UnlockLevels& unlocks = Heartseed::Unlocks();
    if (!Heartseed::SetLevels(unlocks.MajorTiers, ParseList<uint32>(sConfigMgr->GetStringDefault("Heartseed.Majors.TierLevels", "1 25 40 55 70 85"))))
        TC_LOG_ERROR("server.loading", "Heartseed.Majors.TierLevels needs %u ascending levels from 1 to 255; keeping the current levels.", uint32(Heartseed::MaxMajorTiers));
    if (!Heartseed::SetLevels(unlocks.EssenceSlots, ParseList<uint32>(sConfigMgr->GetStringDefault("Heartseed.Essence.SlotLevels", "1 40 70"))))
        TC_LOG_ERROR("server.loading", "Heartseed.Essence.SlotLevels needs %u ascending levels from 1 to 255; keeping the current levels.", uint32(Heartseed::MaxEssenceSlots));
}

void HeartseedMgr::LoadAccounts()
{
    std::set<uint32> accounts;
    if (QueryResult result = CharacterDatabase.Query("SELECT `account` FROM `account_heartseed`"))
    {
        do
            accounts.insert(result->Fetch()[0].GetUInt32());
        while (result->NextRow());
    }
    std::lock_guard<std::mutex> guard(_lock);
    _seedAccounts.swap(accounts);
    TC_LOG_INFO("server.loading", ">> Loaded %u accounts with a Heartseed", uint32(_seedAccounts.size()));
}

bool HeartseedMgr::ShouldLoad(Player const* player) const
{
    if (!player->GetSession()->IsBot())
        return true;
    std::lock_guard<std::mutex> guard(_lock);
    return _seedAccounts.count(player->GetSession()->GetAccountId()) != 0;
}

bool HeartseedMgr::LoadData(std::size_t* loadedRows)
{
    if (loadedRows)
        *loadedRows = 0;
    uint32 oldMSTime = getMSTime();

    // The sentinel distinguishes empty tables from failed queries; failures keep the live cache.
    std::vector<Heartseed::StageInfo> stages;
    std::vector<uint8> minLevels;
    {
        QueryResult result = WorldDatabase.Query("SELECT `stage`, `min_level`, `name`, NULL AS sentinel FROM `heartseed_stage` UNION ALL SELECT NULL, NULL, NULL, 'end' ORDER BY sentinel, `stage`");
        if (!result)
            return false;
        do
        {
            Field* fields = result->Fetch();
            if (!fields[3].IsNull())
                continue;
            uint8 stage = fields[0].GetUInt8();
            if (stage != stages.size())
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_stage`: stages must be numbered 0, 1, 2, ... without gaps; reload aborted at stage %u.", stage);
                return false;
            }

            Heartseed::StageInfo info;
            info.MinLevel = fields[1].GetUInt8();
            info.Name = fields[2].GetString();
            if (!minLevels.empty() && info.MinLevel < minLevels.back())
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_stage`: stage %u starts below the stage before it; reload aborted.", stage);
                return false;
            }

            minLevels.push_back(info.MinLevel);
            stages.push_back(std::move(info));
        } while (result->NextRow());
    }

    std::map<std::pair<uint8, uint8>, uint16> glow;
    {
        QueryResult result = WorldDatabase.Query("SELECT `stage`, `path`, `enchant_id`, NULL AS sentinel FROM `heartseed_glow` UNION ALL SELECT NULL, NULL, NULL, 'end'");
        if (!result)
            return false;
        do
        {
            Field* fields = result->Fetch();
            if (!fields[3].IsNull())
                continue;
            uint8 stage = fields[0].GetUInt8();
            uint8 path = fields[1].GetUInt8();
            uint16 enchantId = fields[2].GetUInt16();
            if (stage >= stages.size() || path >= Heartseed::MaxPaths)
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_glow`: stage %u path %u does not exist, reload aborted.", stage, path);
                return false;
            }
            if (enchantId && !sSpellItemEnchantmentStore.LookupEntry(enchantId))
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_glow`: stage %u path %u uses unknown enchant %u, reload aborted.", stage, path, enchantId);
                return false;
            }
            glow[{ stage, path }] = enchantId;
        } while (result->NextRow());
    }

    std::map<uint32, Heartseed::TraitInfo> traits;
    {
        QueryResult result = WorldDatabase.Query("SELECT `id`, `path`, `kind`, `tier`, `min_rank`, `spell_id`, `base_amount`, `amount_per_ilvl`, `name`, `description`, NULL AS sentinel FROM `heartseed_trait` UNION ALL SELECT NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 'end'");
        if (!result)
            return false;
        do
        {
            Field* fields = result->Fetch();
            if (!fields[10].IsNull())
                continue;
            Heartseed::TraitInfo trait;
            trait.Id = fields[0].GetUInt32();
            uint8 path = fields[1].GetUInt8();
            uint8 kind = fields[2].GetUInt8();
            trait.Tier = fields[3].GetUInt8();
            trait.MinLevel = fields[4].GetUInt8();
            trait.SpellId = fields[5].GetUInt32();
            trait.BaseAmount = fields[6].GetInt32();
            trait.AmountPerItemLevel = fields[7].GetFloat();
            trait.Name = fields[8].GetString();
            trait.Description = fields[9].GetString();

            if (!trait.Id || path >= Heartseed::MaxPaths)
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_trait`: trait %u has id 0 or unknown path %u, reload aborted.", trait.Id, path);
                return false;
            }
            if (kind != uint8(Heartseed::TraitKind::Minor) && kind != uint8(Heartseed::TraitKind::Major))
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_trait`: trait %u has unknown kind %u (1 = minor, 2 = major), reload aborted.", trait.Id, kind);
                return false;
            }
            trait.TraitPath = Heartseed::Path(path);
            trait.Kind = Heartseed::TraitKind(kind);
            if (trait.Kind == Heartseed::TraitKind::Major && trait.Tier >= Heartseed::MaxMajorTiers)
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_trait`: major %u is in tier %u; tiers are 0 to %u. Reload aborted.", trait.Id, trait.Tier, Heartseed::MaxMajorTiers - 1);
                return false;
            }
            SpellInfo const* spellInfo = trait.SpellId ? sSpellMgr->GetSpellInfo(trait.SpellId) : nullptr;
            // Spell 0 is allowed for a major that only teaches a Celestial Gift (heartseed_gift).
            if (trait.SpellId && (!spellInfo || !spellInfo->IsPassive() || !spellInfo->Effects[EFFECT_0].IsAura()))
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_trait`: trait %u uses spell %u, which is missing, not passive or has no aura in effect 0. Reload aborted.", trait.Id, trait.SpellId);
                return false;
            }
            traits[trait.Id] = std::move(trait);
        } while (result->NextRow());
    }

    {
        QueryResult result = WorldDatabase.Query("SELECT `trait_id`, `spell_id`, `cooldown`, `duration`, `base_amount`, `amount_per_ilvl`, NULL AS sentinel FROM `heartseed_gift` UNION ALL SELECT NULL, NULL, NULL, NULL, NULL, NULL, 'end'");
        if (!result)
            return false;
        do
        {
            Field* fields = result->Fetch();
            if (!fields[6].IsNull())
                continue;
            uint32 traitId = fields[0].GetUInt32();
            uint32 spellId = fields[1].GetUInt32();
            auto trait = traits.find(traitId);
            if (trait == traits.end() || trait->second.Kind != Heartseed::TraitKind::Major)
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_gift`: trait %u is missing or not a major, reload aborted.", traitId);
                return false;
            }
            if (!sSpellMgr->GetSpellInfo(spellId))
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_gift`: trait %u uses unknown spell %u, reload aborted.", traitId, spellId);
                return false;
            }
            trait->second.GiftSpellId = spellId;
            trait->second.GiftCooldown = fields[2].GetUInt32();
            trait->second.GiftDuration = fields[3].GetUInt32();
            trait->second.GiftBaseAmount = fields[4].GetInt32();
            trait->second.GiftAmountPerItemLevel = fields[5].GetFloat();
        } while (result->NextRow());
    }

    {
        QueryResult result = WorldDatabase.Query("SELECT `trait_id`, `stage`, `kind`, `amount`, `duration`, `spell_id`, NULL AS sentinel FROM `heartseed_gift_rider` UNION ALL SELECT NULL, NULL, NULL, NULL, NULL, NULL, 'end' ORDER BY `trait_id`, `stage`");
        if (!result)
            return false;
        do
        {
            Field* fields = result->Fetch();
            if (!fields[6].IsNull())
                continue;
            uint32 traitId = fields[0].GetUInt32();
            Heartseed::GiftRider rider;
            rider.Stage = fields[1].GetUInt8();
            uint8 kind = fields[2].GetUInt8();
            rider.Kind = Heartseed::GiftRiderKind(kind);
            rider.Amount = fields[3].GetInt32();
            rider.Duration = fields[4].GetUInt32();
            rider.SpellId = fields[5].GetUInt32();
            auto trait = traits.find(traitId);
            if (trait == traits.end() || !trait->second.GiftSpellId)
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_gift_rider`: trait %u has no Gift, reload aborted.", traitId);
                return false;
            }
            if (rider.Stage >= stages.size() || !kind || kind > Heartseed::MaxGiftRiderKind)
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_gift_rider`: trait %u stage %u has an unknown stage or kind %u, reload aborted.", traitId, rider.Stage, kind);
                return false;
            }
            if (rider.SpellId && !sSpellMgr->GetSpellInfo(rider.SpellId))
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_gift_rider`: trait %u stage %u uses unknown spell %u, reload aborted.", traitId, rider.Stage, rider.SpellId);
                return false;
            }
            trait->second.GiftRiders.push_back(rider);
        } while (result->NextRow());
    }

    std::map<uint8, Heartseed::MeterRule> meters;
    {
        QueryResult result = WorldDatabase.Query("SELECT `path`, `source`, `max_stacks`, `threshold_pct`, `lit_duration`, `decay`, `build_cooldown`, `spend_bonus_pct`, `crit_cooldown_cut`, `window`, NULL AS sentinel FROM `heartseed_meter` UNION ALL SELECT NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 'end'");
        if (!result)
            return false;
        do
        {
            Field* fields = result->Fetch();
            if (!fields[10].IsNull())
                continue;
            uint8 path = fields[0].GetUInt8();
            uint8 source = fields[1].GetUInt8();
            if (!path || path >= Heartseed::MaxPaths || !source || source > Heartseed::MaxMeterSource)
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_meter`: path %u has an unknown path or source %u, reload aborted.", path, source);
                return false;
            }
            Heartseed::MeterRule rule;
            rule.Source = Heartseed::MeterSource(source);
            rule.MaxStacks = fields[2].GetUInt8();
            rule.ThresholdPct = fields[3].GetUInt32();
            rule.LitMs = fields[4].GetUInt32();
            rule.DecayMs = fields[5].GetUInt32();
            rule.BuildCooldownMs = fields[6].GetUInt32();
            rule.SpendBonusPct = fields[7].GetInt32();
            rule.CritCooldownCutMs = fields[8].GetInt32();
            rule.WindowMs = fields[9].GetUInt32();
            meters[path] = rule;
        } while (result->NextRow());
    }

    std::map<uint32, Heartseed::EssenceInfo> essences;
    {
        QueryResult result = WorldDatabase.Query("SELECT `id`, `name`, `proc_spell_id`, `proc_chance`, `weapon_damage_pct`, `description`, NULL AS sentinel FROM `heartseed_essence_template` UNION ALL SELECT NULL, NULL, NULL, NULL, NULL, NULL, 'end'");
        if (!result)
            return false;
        do
        {
            Field* fields = result->Fetch();
            if (!fields[6].IsNull())
                continue;
            Heartseed::EssenceInfo essence;
            essence.Id = fields[0].GetUInt32();
            essence.Name = fields[1].GetString();
            essence.ProcSpellId = fields[2].GetUInt32();
            essence.ProcChance = fields[3].GetFloat();
            essence.WeaponDamagePct = fields[4].GetFloat();
            essence.Description = fields[5].GetString();
            if (!essence.Id)
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_essence_template`: essence id 0 is reserved for the core proc, reload aborted.");
                return false;
            }
            if (essence.ProcSpellId && !sSpellMgr->GetSpellInfo(essence.ProcSpellId))
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_essence_template`: essence %u uses unknown spell %u; reload aborted.", essence.Id, essence.ProcSpellId);
                return false;
            }
            essences[essence.Id] = std::move(essence);
        } while (result->NextRow());
    }

    std::vector<Heartseed::EssenceSource> essenceSources;
    {
        QueryResult result = WorldDatabase.Query("SELECT `essence_id`, `item_entry`, `name_pattern`, `chance`, NULL AS sentinel FROM `heartseed_essence_source` UNION ALL SELECT NULL, NULL, NULL, NULL, 'end'");
        if (!result)
            return false;
        do
        {
            Field* fields = result->Fetch();
            if (!fields[4].IsNull())
                continue;
            Heartseed::EssenceSource source;
            source.EssenceId = fields[0].GetUInt32();
            source.ItemEntry = fields[1].GetUInt32();
            source.NamePattern = ToLower(fields[2].GetString());
            source.Chance = fields[3].GetFloat();
            if (!essences.count(source.EssenceId))
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_essence_source`: essence %u does not exist, reload aborted.", source.EssenceId);
                return false;
            }
            if (!source.ItemEntry && source.NamePattern.empty())
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_essence_source`: a row for essence %u has neither an item entry nor a name pattern, reload aborted.", source.EssenceId);
                return false;
            }
            essenceSources.push_back(std::move(source));
        } while (result->NextRow());
    }

    std::set<uint32> twinkItems;
    {
        QueryResult result = WorldDatabase.Query("SELECT `entry`, NULL AS sentinel FROM `twink_item` UNION ALL SELECT NULL, 'end'");
        if (!result)
            return false;
        do
        {
            Field* fields = result->Fetch();
            if (fields[1].IsNull())
                twinkItems.insert(fields[0].GetUInt32());
        } while (result->NextRow());
    }

    std::vector<Heartseed::ShrineInfo> shrines;
    {
        QueryResult result = WorldDatabase.Query("SELECT `id`, `map`, `position_x`, `position_y`, `position_z`, `radius`, `name`, NULL AS sentinel FROM `heartseed_shrine` UNION ALL SELECT NULL, NULL, NULL, NULL, NULL, NULL, NULL, 'end'");
        if (!result)
            return false;
        do
        {
            Field* fields = result->Fetch();
            if (!fields[7].IsNull())
                continue;
            Heartseed::ShrineInfo shrine;
            shrine.Id = fields[0].GetUInt32();
            shrine.MapId = fields[1].GetUInt32();
            shrine.X = fields[2].GetFloat();
            shrine.Y = fields[3].GetFloat();
            shrine.Z = fields[4].GetFloat();
            shrine.Radius = fields[5].GetFloat();
            shrine.Name = fields[6].GetString();
            if (shrine.Radius <= 0.0f)
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_shrine`: shrine %u has no radius; reload aborted.", shrine.Id);
                return false;
            }
            shrines.push_back(shrine);
        } while (result->NextRow());
    }

    std::map<uint8, Heartseed::GrowthStat> growthStats;
    {
        QueryResult result = WorldDatabase.Query("SELECT `id`, `name`, `main_stat`, `spell_id`, `max_pct`, `half_points`, NULL AS sentinel FROM `heartseed_growth_stat` UNION ALL SELECT NULL, NULL, NULL, NULL, NULL, NULL, 'end'");
        if (!result)
            return false;
        do
        {
            Field* fields = result->Fetch();
            if (!fields[6].IsNull())
                continue;
            Heartseed::GrowthStat stat;
            stat.Id = fields[0].GetUInt8();
            stat.Name = fields[1].GetString();
            stat.MainStat = fields[2].GetUInt8() != 0;
            stat.SpellId = fields[3].GetUInt32();
            stat.MaxPct = fields[4].GetInt32();
            stat.HalfPoints = fields[5].GetUInt32();
            if (!stat.Id || (!stat.MainStat && !sSpellMgr->GetSpellInfo(stat.SpellId)))
            {
                TC_LOG_ERROR("sql.sql", "Table `heartseed_growth_stat`: stat %u has id 0 or unknown spell %u; reload aborted.", uint32(stat.Id), stat.SpellId);
                return false;
            }
            growthStats[stat.Id] = stat;
        } while (result->NextRow());
    }

    if (!sSpellMgr->GetSpellInfo(Heartseed::SPELL_CORE_WEAPON_BONUS))
        TC_LOG_ERROR("sql.sql", "Heartseed: core passive spell %u is missing from `spell_dbc`; no bonus weapon damage.", Heartseed::SPELL_CORE_WEAPON_BONUS);
    if (!sSpellMgr->GetSpellInfo(Heartseed::SPELL_METER_PASSIVE))
        TC_LOG_ERROR("sql.sql", "Heartseed: meter passive spell %u is missing from `spell_dbc`; crits build no meter.", Heartseed::SPELL_METER_PASSIVE);
    if (_procSpellId && !sSpellMgr->GetSpellInfo(_procSpellId))
        TC_LOG_ERROR("server.loading", "Heartseed.Proc.SpellId %u does not exist; the core proc stays off.", _procSpellId);

    if (stages.empty())
    {
        TC_LOG_ERROR("sql.sql", "Heartseed reload aborted: heartseed_stage must contain at least stage 0.");
        return false;
    }
    auto stageCount = stages.size(), glowCount = glow.size(), traitCount = traits.size();
    auto essenceCount = essences.size(), sourceCount = essenceSources.size(), twinkCount = twinkItems.size(), shrineCount = shrines.size();
    auto meterCount = meters.size(), growthStatCount = growthStats.size();
    {
        std::lock_guard<std::mutex> guard(_lock);
        _stages.swap(stages);
        _stageMinLevels.swap(minLevels);
        _glow.swap(glow);
        _traits.swap(traits);
        _essences.swap(essences);
        _essenceSources.swap(essenceSources);
        _twinkItems.swap(twinkItems);
        _shrines.swap(shrines);
        _meters.swap(meters);
        _growthStats.swap(growthStats);
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u Heartseed stages, %u glows, %u traits, %u essences, %u essence sources, %u twink items, %u shrines, %u meters and %u Growth stats in %u ms", uint32(stageCount),
        uint32(glowCount), uint32(traitCount), uint32(essenceCount), uint32(sourceCount), uint32(twinkCount), uint32(shrineCount), uint32(meterCount),
        uint32(growthStatCount), GetMSTimeDiffToNow(oldMSTime));
    if (loadedRows)
        *loadedRows = stageCount + glowCount + traitCount + essenceCount + sourceCount + twinkCount + shrineCount + meterCount + growthStatCount;
    return true;
}

void HeartseedMgr::LoadPlayer(Player* player)
{
    uint32 guid = player->GetGUID().GetCounter();
    uint32 account = player->GetSession()->GetAccountId();

    // Another character of the account already loaded holds the newest shared values; else the database.
    Heartseed::State state;
    bool shared = false;
    {
        std::lock_guard<std::mutex> guard(_lock);
        for (auto const& [otherGuid, other] : _states)
        {
            if (otherGuid == guid || other.AccountId != account)
                continue;
            state.GrantedTime = other.GrantedTime;
            state.FedWeapons = other.FedWeapons;
            for (auto const& [essenceId, held] : other.Essences)
                state.Essences[essenceId] = { held.first, 0 };
            shared = true;
            break;
        }
    }

    Field* fields = nullptr;
    if (!shared)
    {
        // The rank and growth columns are retired (T-083): Growth is the character's own now.
        QueryResult accountResult = CharacterDatabase.PQuery("SELECT `granted_time` FROM `account_heartseed` WHERE `account` = %u", account);
        if (!accountResult)
        {
            UnloadPlayer(player);
            return;
        }
        fields = accountResult->Fetch();
        state.GrantedTime = fields[0].GetUInt32();

        if (QueryResult result = CharacterDatabase.PQuery("SELECT `essence_id`, `count` FROM `account_heartseed_essence` WHERE `account` = %u", account))
        {
            do
            {
                fields = result->Fetch();
                state.Essences[fields[0].GetUInt32()] = { fields[1].GetUInt32(), 0 };
            } while (result->NextRow());
        }

        if (QueryResult result = CharacterDatabase.PQuery("SELECT CAST(COALESCE(SUM(`count`), 0) AS UNSIGNED) FROM `account_heartseed_fed_weapon` WHERE `account` = %u", account))
            state.FedWeapons = uint32(result->Fetch()[0].GetUInt64());
    }
    state.AccountId = account;

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

    // Only the slotting is this character's; an essence the account no longer holds stays out.
    if (QueryResult result = CharacterDatabase.PQuery("SELECT `essence_id`, `active_slot` FROM `heartseed_essence` WHERE `guid` = %u", guid))
    {
        do
        {
            fields = result->Fetch();
            auto essence = state.Essences.find(fields[0].GetUInt32());
            if (essence != state.Essences.end())
                essence->second.second = fields[1].GetUInt8();
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

    if (QueryResult result = CharacterDatabase.PQuery("SELECT `level`, `xp` FROM `character_heartseed_growth` WHERE `guid` = %u", guid))
    {
        fields = result->Fetch();
        state.GrowthLevel = fields[0].GetUInt32();
        state.GrowthXp = fields[1].GetUInt32();
    }

    if (QueryResult result = CharacterDatabase.PQuery("SELECT `stat_id`, `points` FROM `character_heartseed_growth_point` WHERE `guid` = %u", guid))
    {
        do
        {
            fields = result->Fetch();
            if (uint32 points = fields[1].GetUInt32())
                state.GrowthPoints[fields[0].GetUInt8()] = points;
        } while (result->NextRow());
    }

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

    // Loadouts, essences and flags are written as whole sets: they are small and change rarely. The account's
    // shared values are the same in every loaded character, so whichever saves writes them.
    uint32 account = state.AccountId;
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("INSERT INTO `account_heartseed` (`account`, `granted_time`) VALUES (%u, %u) ON DUPLICATE KEY UPDATE `granted_time` = VALUES(`granted_time`)",
        account, state.GrantedTime);
    trans->PAppend("DELETE FROM `account_heartseed_essence` WHERE `account` = %u", account);
    for (auto const& [essenceId, essence] : state.Essences)
        trans->PAppend("INSERT INTO `account_heartseed_essence` (`account`, `essence_id`, `count`) VALUES (%u, %u, %u)",
            account, essenceId, essence.first);

    trans->PAppend("DELETE FROM `heartseed_loadout` WHERE `guid` = %u", guid);
    for (auto const& [specId, traits] : state.Loadouts)
        for (uint8 tier = 0; tier < Heartseed::MaxMajorTiers; ++tier)
            if (traits[tier])
                trans->PAppend("INSERT INTO `heartseed_loadout` (`guid`, `spec_id`, `tier`, `trait_id`) VALUES (%u, %u, %u, %u)",
                    guid, uint32(specId), uint32(tier), traits[tier]);

    // This character's slotting; count is the account's, kept so the row reads on its own.
    trans->PAppend("DELETE FROM `heartseed_essence` WHERE `guid` = %u", guid);
    for (auto const& [essenceId, essence] : state.Essences)
        if (essence.second)
            trans->PAppend("INSERT INTO `heartseed_essence` (`guid`, `essence_id`, `count`, `active_slot`) VALUES (%u, %u, %u, %u)",
                guid, essenceId, essence.first, uint32(essence.second));

    trans->PAppend("DELETE FROM `heartseed_story_flag` WHERE `guid` = %u", guid);
    for (auto const& [flagId, setTime] : state.StoryFlags)
        trans->PAppend("INSERT INTO `heartseed_story_flag` (`guid`, `flag_id`, `set_time`) VALUES (%u, %u, %u)",
            guid, uint32(flagId), setTime);

    trans->PAppend("REPLACE INTO `character_heartseed_growth` (`guid`, `level`, `xp`) VALUES (%u, %u, %u)", guid, state.GrowthLevel, state.GrowthXp);
    trans->PAppend("DELETE FROM `character_heartseed_growth_point` WHERE `guid` = %u", guid);
    for (auto const& [statId, points] : state.GrowthPoints)
        if (points)
            trans->PAppend("INSERT INTO `character_heartseed_growth_point` (`guid`, `stat_id`, `points`) VALUES (%u, %u, %u)",
                guid, uint32(statId), points);

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
        // Another character of the account may already hold the seed (an alt online since before the grant).
        uint32 account = player->GetSession()->GetAccountId();
        itr->second.AccountId = account;
        _seedAccounts.insert(account);
        itr->second.GrantedTime = uint32(time(nullptr));
        for (auto const& [otherGuid, other] : _states)
        {
            if (otherGuid == itr->first || other.AccountId != account)
                continue;
            itr->second.GrantedTime = other.GrantedTime;
            itr->second.FedWeapons = other.FedWeapons;
            for (auto const& [essenceId, held] : other.Essences)
                itr->second.Essences[essenceId] = { held.first, 0 };
            break;
        }
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
    uint32 account = player->GetSession()->GetAccountId();
    // The seed is the account's: every loaded character of it loses its state and auras.
    std::map<uint32, std::map<uint32, int32>> applied;
    {
        std::lock_guard<std::mutex> guard(_lock);
        if (!_states.count(guid))
            return false;
        for (auto itr = _states.begin(); itr != _states.end();)
        {
            if (itr->first == guid || itr->second.AccountId == account)
            {
                applied[itr->first] = std::move(itr->second.AppliedAuras);
                itr = _states.erase(itr);
            }
            else
                ++itr;
        }
        _seedAccounts.erase(account);
    }

    // The pre-T-049 per-character tables go too, so running the migration again cannot bring the seed back.
    std::string characters = "SELECT `guid` FROM `characters` WHERE `account` = " + std::to_string(account);
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM `account_heartseed` WHERE `account` = %u", account);
    trans->PAppend("DELETE FROM `account_heartseed_essence` WHERE `account` = %u", account);
    trans->PAppend("DELETE FROM `account_heartseed_fed_weapon` WHERE `account` = %u", account);
    for (char const* table : { "heartseed_state", "heartseed_loadout", "heartseed_essence", "heartseed_story_flag", "heartseed_fed_weapon",
        "character_heartseed_growth", "character_heartseed_growth_point" })
        trans->PAppend("DELETE FROM `%s` WHERE `guid` IN (%s)", table, characters.c_str());
    // The quest is the canonical marker, so revoking makes it available again on every character.
    trans->PAppend("DELETE FROM `character_queststatus_rewarded` WHERE `quest` = %u AND `guid` IN (%s)", _grantQuestId, characters.c_str());
    CharacterDatabase.CommitTransaction(trans);

    for (auto const& [revokedGuid, auras] : applied)
    {
        Player* revoked = revokedGuid == guid ? player : ObjectAccessor::FindConnectedPlayer(ObjectGuid::Create<HighGuid::Player>(revokedGuid));
        if (!revoked)
            continue;
        RemoveAuras(revoked, auras);
        SyncGifts(revoked, {}, false);
        if (revoked->GetQuestStatus(_grantQuestId) != QUEST_STATUS_NONE)
            revoked->RemoveActiveQuest(_grantQuestId, true, true);
        revoked->RemoveRewardedQuest(_grantQuestId);
        RefreshGlow(revoked);
    }
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
        ShareAccountState(itr->first, itr->second);
    }

    // Saved now rather than with the character, so a crash or a GM test never loses the change.
    SavePlayer(player);
    Refresh(player);
    return true;
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

bool HeartseedMgr::HasUnchosenOpenTier(Player* player) const
{
    Heartseed::State state;
    if (!GetState(player, state))
        return false;
    auto loadout = state.Loadouts.find(uint16(player->GetTalentSpecialization()));
    for (uint8 tier = 0; tier < Heartseed::MaxMajorTiers; ++tier)
        if (Heartseed::MajorTierOpen(tier, player->GetLevel()) && (loadout == state.Loadouts.end() || !loadout->second[tier])
            && TierHasMajors(tier))
            return true;
    return false;
}

bool HeartseedMgr::TierHasMajors(uint8 tier) const
{
    std::lock_guard<std::mutex> guard(_lock);
    for (auto const& [id, trait] : _traits)
        if (trait.Kind == Heartseed::TraitKind::Major && trait.Tier == tier)
            return true;
    return false;
}

void HeartseedMgr::ScheduleChoicePrompt(Player* player)
{
    if (_choicePromptOnLogin && HasUnchosenOpenTier(player))
        player->m_Events.AddEvent(new HeartseedChoicePromptEvent(player), player->m_Events.CalculateTime(_choicePromptDelay));
}

bool HeartseedMgr::GetShrineAt(Player const* player, Heartseed::ShrineInfo& out) const
{
    std::lock_guard<std::mutex> guard(_lock);
    for (Heartseed::ShrineInfo const& shrine : _shrines)
    {
        if (shrine.MapId == player->GetMapId() && player->IsWithinDist3d(shrine.X, shrine.Y, shrine.Z, shrine.Radius))
        {
            out = shrine;
            return true;
        }
    }
    return false;
}

std::vector<Heartseed::ShrineInfo> HeartseedMgr::GetShrines() const
{
    std::lock_guard<std::mutex> guard(_lock);
    return _shrines;
}

void HeartseedMgr::SendChoiceMenu(Player* player)
{
    Heartseed::State state;
    if (!GetState(player, state))
        return;
    auto loadout = state.Loadouts.find(uint16(player->GetTalentSpecialization()));
    Heartseed::ShrineInfo shrine;
    bool canChange = !_choiceShrineOnly || GetShrineAt(player, shrine);

    player->PlayerTalkClass->ClearMenus();
    GossipMenu& menu = player->PlayerTalkClass->GetGossipMenu();
    menu.SetMenuId(Heartseed::GOSSIP_MENU_CHOICE);
    for (uint8 tier = 0; tier < Heartseed::MaxMajorTiers; ++tier)
    {
        std::string text = "Level " + std::to_string(Heartseed::MajorTierLevel(tier)) + ": ";
        if (!Heartseed::MajorTierOpen(tier, player->GetLevel()))
        {
            // Only the next locked tier is shown, as something to look forward to.
            menu.AddMenuItem(-1, GOSSIP_ICON_DOT, text + "not yet.", Heartseed::CHOICE_SENDER_CLOSE, 0, "", 0);
            break;
        }

        uint32 chosen = loadout != state.Loadouts.end() ? loadout->second[tier] : 0;
        Heartseed::TraitInfo trait;
        if (chosen && GetTrait(chosen, trait))
            text += trait.Name + " (" + PathName(trait.TraitPath) + ")" + (canChange ? ". Change it." : ".");
        else if (!TierHasMajors(tier))
        {
            menu.AddMenuItem(-1, GOSSIP_ICON_DOT, text + "the Celestials are silent here for now.", Heartseed::CHOICE_SENDER_BACK, 0, "", 0);
            continue;
        }
        else
            text += "choose a path.";
        menu.AddMenuItem(-1, chosen ? GOSSIP_ICON_TRAINER : GOSSIP_ICON_CHAT, text, Heartseed::CHOICE_SENDER_TIER, tier, "", 0);
    }
    menu.AddMenuItem(-1, GOSSIP_ICON_CHAT, "Let it rest.", Heartseed::CHOICE_SENDER_CLOSE, 0, "", 0);
    player->PlayerTalkClass->SendGossipMenu(Heartseed::NPC_TEXT_CHOICE, player->GetGUID());
}

void HeartseedMgr::SendChoiceTierMenu(Player* player, uint8 tier)
{
    player->PlayerTalkClass->ClearMenus();
    GossipMenu& menu = player->PlayerTalkClass->GetGossipMenu();
    menu.SetMenuId(Heartseed::GOSSIP_MENU_CHOICE);
    for (Heartseed::TraitInfo const& trait : GetTraits())
    {
        if (trait.Kind != Heartseed::TraitKind::Major || trait.Tier != tier)
            continue;
        std::string path = PathName(trait.TraitPath);
        std::string confirm = "Walk the path of " + path + " with " + trait.Name + "?"
            + (_choiceShrineOnly ? " Once chosen, it can be changed only at a Celestial shrine." : "");
        menu.AddMenuItem(-1, GOSSIP_ICON_CHAT, path + ": " + trait.Name + ". " + trait.Description,
            Heartseed::CHOICE_SENDER_PICK, tier * Heartseed::CHOICE_PICK_STRIDE + trait.Id, confirm, 0);
    }
    menu.AddMenuItem(-1, GOSSIP_ICON_DOT, "Back.", Heartseed::CHOICE_SENDER_BACK, 0, "", 0);
    player->PlayerTalkClass->SendGossipMenu(Heartseed::NPC_TEXT_CHOICE_TIER, player->GetGUID());
}

void HeartseedMgr::OnChoiceGossipSelect(Player* player, uint32 sender, uint32 action)
{
    Heartseed::State state;
    if (!GetState(player, state))
    {
        player->PlayerTalkClass->SendCloseGossip();
        return;
    }
    auto loadout = state.Loadouts.find(uint16(player->GetTalentSpecialization()));
    auto chosenAt = [&](uint8 tier) { return loadout != state.Loadouts.end() ? loadout->second[tier] : 0u; };
    Heartseed::ShrineInfo shrine;
    auto lockedHere = [&](uint8 tier) { return chosenAt(tier) && _choiceShrineOnly && !GetShrineAt(player, shrine); };
    ChatHandler handler(player->GetSession());

    switch (sender)
    {
        case Heartseed::CHOICE_SENDER_TIER:
        {
            uint8 tier = uint8(action);
            if (tier >= Heartseed::MaxMajorTiers || !Heartseed::MajorTierOpen(tier, player->GetLevel()))
                break;
            if (lockedHere(tier))
            {
                handler.SendSysMessage("Your Heartseed holds to the path it took. Visit a Celestial shrine to change it.");
                SendChoiceMenu(player);
                return;
            }
            SendChoiceTierMenu(player, tier);
            return;
        }
        case Heartseed::CHOICE_SENDER_PICK:
        {
            uint8 tier = uint8(action / Heartseed::CHOICE_PICK_STRIDE);
            uint32 traitId = action % Heartseed::CHOICE_PICK_STRIDE;
            // Checked again: the player may have walked away from the shrine with the window open.
            if (tier >= Heartseed::MaxMajorTiers || lockedHere(tier))
                break;
            Heartseed::TraitInfo trait;
            if (ChooseMajor(player, tier, traitId, false) == Heartseed::ChoiceResult::Ok && GetTrait(traitId, trait))
                handler.PSendSysMessage("Your Heartseed turns toward %s: %s.", PathName(trait.TraitPath), trait.Name.c_str());
            SendChoiceMenu(player);
            return;
        }
        case Heartseed::CHOICE_SENDER_BACK:
            SendChoiceMenu(player);
            return;
        default:
            break;
    }
    player->PlayerTalkClass->SendCloseGossip();
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
    {
        ChatHandler(player->GetSession()).SendSysMessage("A Heartseed has taken root in your weapon.");
        // The Gift tier opens at 1 by default: the seed's first act is to offer its power, once the reward
        // window has closed.
        if (!player->GetSession()->IsBot())
            player->m_Events.AddEvent(new HeartseedChoicePromptEvent(player), player->m_Events.CalculateTime(GetChoicePromptDelay()));
    }

    // Questing is how a fresh character levels, so it feeds Growth too. Repeatable turn-ins other than dailies and
    // weeklies could be farmed, and quests far below the character are skipped like gray bosses.
    if (quest->IsRepeatable() && !quest->IsDailyOrWeekly())
        return;
    if (Heartseed::GrowthSourceNearLevel(player->GetLevel(), uint8(std::clamp(player->GetQuestLevel(quest), 0, 255)), _growthLevelRange))
        AddGrowthXp(player, _growthQuest);
}

bool HeartseedMgr::ReachStoryBeat(Player* player, uint16 flag)
{
    Heartseed::State state;
    if (!GetState(player, state) || state.StoryFlags.count(flag))
        return false;

    // Story beats unlock lore and cosmetics later, never power (DESIGN.md, Balance stays per character).
    ChatHandler(player->GetSession()).PSendSysMessage("Your Heartseed awakens to this (%s).", GetStoryFlagName(flag));
    if (_awakeningSound)
        player->PlayDirectSound(_awakeningSound, player);
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

    // Every credited group member earns for a rare out in the world, or a dungeon or raid boss, near its own level.
    if (!Heartseed::GrowthSourceNearLevel(player->GetLevel(), killed->GetLevel(), _growthLevelRange))
        return;
    uint32 rank = killed->GetCreatureTemplate()->rank;
    if (rank == CREATURE_ELITE_RARE || rank == CREATURE_ELITE_RAREELITE)
    {
        AddGrowthXp(player, _growthRare);
        return;
    }

    Map const* map = killed->GetMap();
    if (!map || !map->IsDungeon() || !(killed->IsDungeonBoss() || killed->isWorldBoss()))
        return;
    AddGrowthXp(player, map->IsRaid() ? _growthRaidBoss : _growthDungeonBoss);
}

void HeartseedMgr::OnBattlegroundEnd(Player* player, bool won)
{
    // A battleground's bracket is the character's own, so a win is always near its level.
    if (won)
        AddGrowthXp(player, _growthBattlegroundWin);
}

void HeartseedMgr::ShareAccountState(uint32 guid, Heartseed::State const& from)
{
    for (auto& [otherGuid, other] : _states)
    {
        if (otherGuid == guid || other.AccountId != from.AccountId)
            continue;
        other.GrantedTime = from.GrantedTime;
        other.FedWeapons = from.FedWeapons;
        // Counts follow the account; each character keeps its own slots, and loses an essence the account lost.
        std::map<uint32, std::pair<uint32, uint8>> essences;
        for (auto const& [essenceId, held] : from.Essences)
        {
            auto own = other.Essences.find(essenceId);
            essences[essenceId] = { held.first, own != other.Essences.end() ? own->second.second : uint8(0) };
        }
        other.Essences.swap(essences);
    }
}

uint32 HeartseedMgr::GetGrowthXpForLevel(uint32 level) const
{
    return Heartseed::GrowthXpForLevel(level, _growthXpBase, _growthXpPerLevel);
}

uint32 HeartseedMgr::AddGrowthXp(Player* player, uint32 amount)
{
    // Only the character being played earns; the player's alts logged in as playerbots use their points but never
    // grow. An XP-locked twink keeps the balance of its bracket unless Heartseed.Growth.TwinksEarn is on.
    if (!amount || !_enabled || player->GetSession()->IsBot() || !HasHeartseed(player))
        return 0;
    if (!_growthTwinksEarn && player->HasFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN))
        return 0;

    uint32 oldLevel = 0;
    uint32 newLevel = 0;
    uint32 unspent = 0;
    bool firstPoint = false;
    ModifyState(player, [&](Heartseed::State& state)
    {
        oldLevel = state.GrowthLevel;
        Heartseed::GrowthProgress progress = Heartseed::ApplyGrowthXp(state.GrowthLevel, Heartseed::AddCapped(state.GrowthXp, amount),
            _growthXpBase, _growthXpPerLevel);
        state.GrowthLevel = progress.Level;
        state.GrowthXp = progress.Xp;
        newLevel = state.GrowthLevel;
        unspent = Heartseed::UnspentPoints(state.GrowthLevel, state.GrowthPoints);
        if (newLevel > oldLevel && !state.StoryFlags.count(Heartseed::HINT_GROWTH_POINTS))
        {
            state.StoryFlags[Heartseed::HINT_GROWTH_POINTS] = uint32(time(nullptr));
            firstPoint = true;
        }
    });

    ChatHandler handler(player->GetSession());
    if (newLevel > oldLevel)
    {
        handler.PSendSysMessage("Your Heartseed grows: Growth level %u. %u stat point%s to spend.", newLevel, unspent, unspent == 1 ? "" : "s");
        TC_LOG_DEBUG("scripts", "Heartseed: %s Growth level %u -> %u.", player->GetName().c_str(), oldLevel, newLevel);
    }
    if (firstPoint)
    {
        // The system's introduction: the first point is when it means something to the player.
        player->GetSession()->SendNotification("Your Heartseed has grown. Spend its stat point in the Heartseed panel (/hs).");
        handler.SendSysMessage("Quests, rares, battleground wins and dungeon and raid bosses near your level feed your Heartseed's Growth. Each Growth "
            "level is a stat point for stamina, your main stat or a secondary: spend it in the Heartseed panel (/hs) or with .seed spend. "
            "A Celestial shrine takes them back so you can spend them again.");
    }
    return newLevel - oldLevel;
}

bool HeartseedMgr::SetGrowthLevel(Player* player, uint32 level, uint32 xp)
{
    return ModifyState(player, [level, xp](Heartseed::State& state)
    {
        state.GrowthLevel = level;
        state.GrowthXp = xp;
        // A lower level takes back the points it no longer pays for, from the most invested stat first.
        uint64 spent = 0;
        for (auto const& [statId, points] : state.GrowthPoints)
            spent += points;
        while (spent > level && !state.GrowthPoints.empty())
        {
            auto most = std::max_element(state.GrowthPoints.begin(), state.GrowthPoints.end(),
                [](auto const& a, auto const& b) { return a.second < b.second; });
            uint32 take = uint32(std::min<uint64>(most->second, spent - level));
            most->second -= take;
            spent -= take;
            if (!most->second)
                state.GrowthPoints.erase(most);
        }
    });
}

Heartseed::GrowthResult HeartseedMgr::SpendGrowthPoints(Player* player, uint8 statId, uint32 count)
{
    Heartseed::GrowthStat stat;
    if (!GetGrowthStat(statId, stat))
        return Heartseed::GrowthResult::UnknownStat;

    Heartseed::GrowthResult result = Heartseed::GrowthResult::NoHeartseed;
    ModifyState(player, [&](Heartseed::State& state)
    {
        uint32 unspent = Heartseed::UnspentPoints(state.GrowthLevel, state.GrowthPoints);
        if (!unspent || !count)
        {
            result = Heartseed::GrowthResult::NoPoints;
            return;
        }
        uint32& points = state.GrowthPoints[statId];
        points = Heartseed::AddCapped(points, std::min(count, unspent));
        result = Heartseed::GrowthResult::Ok;
    });
    return result;
}

Heartseed::GrowthResult HeartseedMgr::ResetGrowthPoints(Player* player, bool anywhere)
{
    Heartseed::State state;
    if (!GetState(player, state))
        return Heartseed::GrowthResult::NoHeartseed;
    if (state.GrowthPoints.empty())
        return Heartseed::GrowthResult::NoPoints;
    Heartseed::ShrineInfo shrine;
    if (!anywhere && _growthResetAtShrineOnly && !GetShrineAt(player, shrine))
        return Heartseed::GrowthResult::NotAtShrine;
    ModifyState(player, [](Heartseed::State& s) { s.GrowthPoints.clear(); });
    return Heartseed::GrowthResult::Ok;
}

std::vector<Heartseed::GrowthStat> HeartseedMgr::GetGrowthStats() const
{
    std::lock_guard<std::mutex> guard(_lock);
    std::vector<Heartseed::GrowthStat> stats;
    for (auto const& [statId, stat] : _growthStats)
        stats.push_back(stat);
    return stats;
}

bool HeartseedMgr::GetGrowthStat(uint8 statId, Heartseed::GrowthStat& out) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _growthStats.find(statId);
    if (itr == _growthStats.end())
        return false;
    out = itr->second;
    return true;
}

uint32 HeartseedMgr::GetGrowthStatSpell(Player const* player, Heartseed::GrowthStat const& stat)
{
    if (!stat.MainStat)
        return stat.SpellId;
    switch (Heartseed::MainStatFor(player->GetClass(), player->GetTalentSpecialization()))
    {
        case Heartseed::MainStat::Strength: return Heartseed::SPELL_GROWTH_STRENGTH;
        case Heartseed::MainStat::Agility:  return Heartseed::SPELL_GROWTH_AGILITY;
        default:                            return Heartseed::SPELL_GROWTH_INTELLECT;
    }
}

bool HeartseedMgr::CanFeedQuality(uint32 quality) const
{
    std::lock_guard<std::mutex> guard(_lock);
    return _feedQualities.count(quality) != 0;
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
    if (proto->Bonding == BIND_QUEST || proto->StartQuest || !CanFeedQuality(proto->Quality))
        return Heartseed::FeedResult::Excluded;
    // A refundable weapon still buys back its price, which would make vendors an essence shop.
    if (item->HasFlag(ITEM_FIELD_DYNAMIC_FLAGS, ITEM_FLAG_REFUNDABLE))
        return Heartseed::FeedResult::Refundable;
    if (item->IsEquipped())
        return Heartseed::FeedResult::Equipped;
    // Twink gear is kept on purpose; only deleting it (or a GM) feeds it.
    if (source == Heartseed::FeedSource::Replace && IsTwinkItem(proto->ItemId))
        return Heartseed::FeedResult::Twink;
    return Heartseed::FeedResult::Ok;
}

void HeartseedMgr::FeedPreview(ItemTemplate const* proto, uint32 /*itemLevel*/, Heartseed::FeedOutcome& out) const
{
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

    RecordAppearance(player, proto, Heartseed::APPEARANCE_FED);
    // Consumed first, so nothing below can leave the weapon behind after it has paid out. A deleted item is
    // destroyed by the delete handler right after this returns.
    if (source != Heartseed::FeedSource::Delete)
        player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);

    // Every weapon fed is kept by entry: consumed weapons become appearances later (DESIGN.md, Leveling).
    CharacterDatabase.PExecute("INSERT INTO `account_heartseed_fed_weapon` (`account`, `item_entry`, `count`, `first_time`) VALUES (%u, %u, 1, %u) "
        "ON DUPLICATE KEY UPDATE `count` = `count` + 1", before.AccountId, entry, uint32(time(nullptr)));

    bool firstFeed = before.FedWeapons == 0;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(player->GetGUID().GetCounter());
        if (itr != _states.end())
        {
            ++itr->second.FedWeapons;
            ShareAccountState(itr->first, itr->second);
        }
    }

    ChatHandler handler(player->GetSession());
    handler.PSendSysMessage("Your Heartseed consumes %s. Its look stays with the seed.", itemName.c_str());
    if (firstFeed)
        handler.SendSysMessage("The Heartseed feeds on the weapons you outgrow. When a better weapon replaces your main hand, it asks to eat "
            "the old one and keeps its look; essences of a weapon's origin may stay behind in it.");

    for (uint32 essenceId : out.Essences)
    {
        AddEssence(player, essenceId, 1);
        Heartseed::EssenceInfo essence;
        if (GetEssence(essenceId, essence))
            handler.PSendSysMessage("Something of its origin stays behind: %s.", essence.Name.c_str());
    }

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
    menu.AddMenuItem(-1, GOSSIP_ICON_CHAT, "Feed " + name + " to the Heartseed (keeps its look).",
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
        { Heartseed::HINT_GROWTH_POINTS,          "hint_growth_points" },
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

bool HeartseedMgr::SetSpecMajors(Player* player, uint16 specId, std::array<uint32, Heartseed::MaxMajorTiers> const& majors)
{
    return ModifyState(player, [specId, &majors](Heartseed::State& state)
    {
        bool any = std::any_of(majors.begin(), majors.end(), [](uint32 traitId) { return traitId != 0; });
        if (any)
            state.Loadouts[specId] = majors;
        else
            state.Loadouts.erase(specId);   // no row, so RefreshTraits still copies the last spec's choices
    });
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
    std::set<uint32> gifts;
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
        uint32 giftTraitId = 0;

        if (_enabled)
        {
            std::array<uint8, Heartseed::MaxPaths> majors = MajorsPerPath(state, specId);
            auto loadout = state.Loadouts.find(specId);
            for (auto const& [traitId, trait] : _traits)
            {
                bool active = trait.Kind == Heartseed::TraitKind::Minor
                    ? Heartseed::MinorActive(trait.TraitPath, trait.MinLevel, player->GetLevel(), majors)
                    : loadout != state.Loadouts.end() && loadout->second[trait.Tier] == traitId;
                if (!active)
                    continue;
                if (trait.SpellId)
                    desired[trait.SpellId] += Heartseed::TraitAmount(trait.BaseAmount, trait.AmountPerItemLevel, itemLevel);
                if (trait.GiftSpellId)
                {
                    gifts.insert(trait.GiftSpellId);
                    if (!giftTraitId)
                        giftTraitId = traitId;
                }
            }
            if (weaponBonus > 0)
                desired[Heartseed::SPELL_CORE_WEAPON_BONUS] += weaponBonus;
            if (giftTraitId)
                desired[Heartseed::SPELL_METER_PASSIVE] = 0;

            // Growth points, as a percentage of the stat (T-083).
            for (auto const& [statId, points] : state.GrowthPoints)
            {
                auto stat = _growthStats.find(statId);
                if (stat == _growthStats.end())
                    continue;
                if (int32 amount = Heartseed::StatPointAmount(points, stat->second.MaxPct, stat->second.HalfPoints))
                    desired[GetGrowthStatSpell(player, stat->second)] += amount;
            }
        }

        // A new Gift starts with an empty meter.
        if (state.GiftTraitId != giftTraitId)
        {
            state.GiftTraitId = giftTraitId;
            state.Meter = Heartseed::MeterState();
            state.RecentHits.clear();
            state.LastSpend = Heartseed::GiftSpend();
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

    SyncGifts(player, gifts, true);

    if (copiedLoadout)
        SavePlayer(player);
}

void HeartseedMgr::SyncGifts(Player* player, std::set<uint32> const& wanted, bool announce)
{
    std::set<uint32> all;
    std::map<uint32, std::string> names;
    {
        std::lock_guard<std::mutex> guard(_lock);
        for (auto const& [traitId, trait] : _traits)
            if (trait.GiftSpellId)
            {
                all.insert(trait.GiftSpellId);
                names[trait.GiftSpellId] = trait.Name;
            }
        auto itr = _states.find(player->GetGUID().GetCounter());
        if (itr != _states.end())
            itr->second.TaughtGifts = wanted;
    }

    // Gifts are clones of client spells (T-086); before that they were the client spells themselves, some of them
    // monk talents, so the old ones are taken back too unless a monk took the talent.
    for (uint32 spellId : Heartseed::RETIRED_GIFT_SPELLS)
        if (player->HasSpell(spellId) && !player->HasTalent(spellId, player->GetActiveSpec()))
            player->RemoveSpell(spellId);

    for (uint32 spellId : all)
    {
        bool has = player->HasSpell(spellId);
        if (wanted.count(spellId))
        {
            if (has)
                continue;
            player->LearnSpell(spellId, false);
            // The trait's name: Gift clones have no name on the server, only in the client patch.
            if (announce)
                ChatHandler(player->GetSession()).PSendSysMessage("|cff40ff40Your Heartseed stirs. A Celestial Gift wakes: %s. Find it in your spellbook.|r",
                    names[spellId].c_str());
        }
        else if (has && !player->HasTalent(spellId, player->GetActiveSpec()))
            player->RemoveSpell(spellId);
    }
}

bool HeartseedMgr::GetActiveGift(Player const* player, uint32 spellId, Heartseed::TraitInfo& out) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto state = _states.find(player->GetGUID().GetCounter());
    if (state == _states.end() || (spellId && !state->second.TaughtGifts.count(spellId)))
        return false;
    auto loadout = state->second.Loadouts.find(uint16(player->GetTalentSpecialization()));
    if (loadout == state->second.Loadouts.end())
        return false;
    for (uint32 traitId : loadout->second)
    {
        auto trait = _traits.find(traitId);
        if (trait == _traits.end() || !trait->second.GiftSpellId)
            continue;
        if (spellId ? trait->second.GiftSpellId == spellId : state->second.TaughtGifts.count(trait->second.GiftSpellId) > 0)
        {
            out = trait->second;
            return true;
        }
    }
    return false;
}

int32 HeartseedMgr::GetGiftAmount(Player const* player, Heartseed::TraitInfo const& gift) const
{
    // A share of the character's own power, so the Gift is as strong at 12 as at 90 relative to the character.
    float attackPower = std::max(player->GetTotalAttackPowerValue(BASE_ATTACK), player->GetTotalAttackPowerValue(RANGED_ATTACK));
    float spellPower = float(player->SpellBaseDamageBonusDone(SPELL_SCHOOL_MASK_ALL));
    uint32 power = uint32(std::max(0.0f, std::max(attackPower, spellPower)));
    Item* weapon = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
    uint32 itemLevel = weapon && !weapon->IsBroken() && weapon->GetTemplate()->Class == ITEM_CLASS_WEAPON ? weapon->GetItemLevel() : 0;
    return int32(Heartseed::PercentOf(power, gift.GiftBaseAmount)) + Heartseed::TraitAmount(0, gift.GiftAmountPerItemLevel, itemLevel);
}

std::vector<Heartseed::GiftRider> HeartseedMgr::GetGiftRiders(Player const* player, Heartseed::TraitInfo const& gift) const
{
    return Heartseed::ActiveRiders(gift.GiftRiders, GetStage(player));
}

bool HeartseedMgr::GetGiftRider(Player const* player, uint32 spellId, Heartseed::GiftRiderKind kind, Heartseed::GiftRider& out) const
{
    Heartseed::TraitInfo gift;
    if (!GetActiveGift(player, spellId, gift))
        return false;
    for (Heartseed::GiftRider const& rider : GetGiftRiders(player, gift))
    {
        if (rider.Kind == kind)
        {
            out = rider;
            return true;
        }
    }
    return false;
}

void HeartseedMgr::CutGiftCooldown(Player* player, uint32 spellId, int32 ms) const
{
    uint32 cut = Heartseed::CooldownCut(player->GetSpellCooldownDelay(spellId), ms);
    if (cut)
        player->ModifySpellCooldown(spellId, -int32(cut));
}

void HeartseedMgr::OnMeterEvent(Player* player, Heartseed::MeterSource source, uint32 amount, uint32 spellId)
{
    if (!_enabled)
        return;

    uint32 now = getMSTime();
    uint32 giftSpellId = 0;
    int32 cut = 0;
    bool changed = false;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(player->GetGUID().GetCounter());
        if (itr == _states.end() || !itr->second.GiftTraitId)
            return;
        Heartseed::State& state = itr->second;
        auto trait = _traits.find(state.GiftTraitId);
        if (trait == _traits.end())
            return;
        auto meter = _meters.find(uint8(trait->second.TraitPath));
        if (meter == _meters.end())
            return;
        Heartseed::MeterRule const& rule = meter->second;
        giftSpellId = trait->second.GiftSpellId;
        // The Gift's own heals and casts never feed the meter it just spent.
        if (spellId && spellId == giftSpellId)
            return;

        switch (source)
        {
            case Heartseed::MeterSource::Crit:
                cut = Heartseed::CritCut(state.Meter, rule, now);
                if (rule.Source == source)
                    changed = Heartseed::MeterBuild(state.Meter, rule, now);
                break;
            case Heartseed::MeterSource::Healing:
                if (rule.Source == source)
                    if (uint8 stacks = Heartseed::HealingStacks(state.Meter, rule, amount, player->GetMaxHealth()))
                        changed = Heartseed::MeterBuild(state.Meter, rule, now, stacks);
                break;
            case Heartseed::MeterSource::BigHit:
                if (rule.WindowMs)
                {
                    state.RecentHits.emplace_back(now, amount);
                    Heartseed::RecentDamage(state.RecentHits, now, rule.WindowMs);
                }
                if (rule.Source == source && Heartseed::IsBigHit(rule, amount, player->GetMaxHealth()))
                    changed = Heartseed::MeterBuild(state.Meter, rule, now);
                break;
            case Heartseed::MeterSource::SpellCast:
                if (rule.Source == source)
                    changed = Heartseed::MeterBuild(state.Meter, rule, now);
                break;
            default:
                break;
        }
    }

    if (cut)
        CutGiftCooldown(player, giftSpellId, cut);
    if (changed)
        SendMeter(player);
}

bool HeartseedMgr::CanCastGift(Player const* player, uint32 spellId) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _states.find(player->GetGUID().GetCounter());
    if (itr == _states.end() || !itr->second.GiftTraitId)
        return true;
    auto trait = _traits.find(itr->second.GiftTraitId);
    if (trait == _traits.end() || trait->second.GiftSpellId != spellId)
        return true;
    auto meter = _meters.find(uint8(trait->second.TraitPath));
    return meter == _meters.end() || Heartseed::MeterAllowsCast(itr->second.Meter, meter->second, getMSTime());
}

Heartseed::GiftSpend HeartseedMgr::SpendGiftMeter(Player* player, uint32 spellId)
{
    Heartseed::GiftSpend spend;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(player->GetGUID().GetCounter());
        if (itr == _states.end() || !itr->second.GiftTraitId)
            return spend;
        Heartseed::State& state = itr->second;
        auto trait = _traits.find(state.GiftTraitId);
        if (trait == _traits.end() || trait->second.GiftSpellId != spellId)
            return spend;
        auto meter = _meters.find(uint8(trait->second.TraitPath));
        if (meter == _meters.end())
            return spend;
        uint32 now = getMSTime();
        spend.Stacks = Heartseed::MeterSpend(state.Meter, meter->second, now);
        spend.BonusPctPerStack = meter->second.SpendBonusPct;
        if (meter->second.WindowMs)
            spend.RecentDamage = Heartseed::RecentDamage(state.RecentHits, now, meter->second.WindowMs);
        state.LastSpend = spend;
    }
    SendMeter(player);
    return spend;
}

Heartseed::GiftSpend HeartseedMgr::GetLastSpend(Player const* player) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _states.find(player->GetGUID().GetCounter());
    return itr == _states.end() ? Heartseed::GiftSpend() : itr->second.LastSpend;
}

std::vector<Heartseed::TraitInfo> HeartseedMgr::GetGiftTraits() const
{
    std::lock_guard<std::mutex> guard(_lock);
    std::vector<Heartseed::TraitInfo> gifts;
    for (auto const& [traitId, trait] : _traits)
        if (trait.GiftSpellId)
            gifts.push_back(trait);
    return gifts;
}

bool HeartseedMgr::GetMeterRule(uint8 path, Heartseed::MeterRule& rule) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto found = _meters.find(path);
    if (found == _meters.end())
        return false;
    rule = found->second;
    return true;
}

bool HeartseedMgr::GetMeter(Player const* player, Heartseed::MeterRule& rule, Heartseed::MeterState& meter) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _states.find(player->GetGUID().GetCounter());
    if (itr == _states.end() || !itr->second.GiftTraitId)
        return false;
    auto trait = _traits.find(itr->second.GiftTraitId);
    if (trait == _traits.end())
        return false;
    auto found = _meters.find(uint8(trait->second.TraitPath));
    if (found == _meters.end())
        return false;
    rule = found->second;
    meter = itr->second.Meter;
    Heartseed::MeterDecay(meter, rule, getMSTime());
    return true;
}

bool HeartseedMgr::ForceMeter(Player* player, uint8 stacks, bool lit)
{
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(player->GetGUID().GetCounter());
        if (itr == _states.end() || !itr->second.GiftTraitId)
            return false;
        auto trait = _traits.find(itr->second.GiftTraitId);
        if (trait == _traits.end())
            return false;
        auto meter = _meters.find(uint8(trait->second.TraitPath));
        if (meter == _meters.end())
            return false;
        Heartseed::MeterState& state = itr->second.Meter;
        uint32 now = getMSTime();
        state.Stacks = std::min(stacks, meter->second.MaxStacks);
        state.LastBuild = now;
        state.HasBuilt = true;
        if (lit && meter->second.LitMs)
        {
            state.Lit = true;
            state.LitAt = now;
        }
    }
    SendMeter(player);
    return true;
}

void HeartseedMgr::SendMeter(Player* player) const
{
    if (player->GetSession()->IsBot())
        return;
    Heartseed::MeterRule rule;
    Heartseed::MeterState meter;
    uint32 giftSpellId = 0;
    if (!GetMeter(player, rule, meter))
        return;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(player->GetGUID().GetCounter());
        if (itr != _states.end())
            if (auto trait = _traits.find(itr->second.GiftTraitId); trait != _traits.end())
                giftSpellId = trait->second.GiftSpellId;
    }
    uint32 litLeft = meter.Lit ? rule.LitMs - std::min(rule.LitMs, getMSTimeDiff(meter.LitAt, getMSTime())) : 0;
    std::string message = Trinity::StringFormat("P\t%u\t%u\t%u\t%u\t%u", uint32(meter.Stacks), uint32(rule.MaxStacks), litLeft,
        rule.DecayMs, giftSpellId);
    WorldPacket data;
    ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, player, player, message, 0, "", DEFAULT_LOCALE, "Heartseed");
    player->SendDirectMessage(&data);
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
