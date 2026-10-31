/*
 * Heartseed state and data. See HeartseedMgr.h and README.md.
 */

#include "HeartseedMgr.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "DBCStores.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "Random.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "Timer.h"
#include "WorldSession.h"

#include <ctime>

namespace
{
    // Set while the proc deals its damage, so that damage never rolls another proc.
    thread_local bool InProc = false;

    // Deals the core proc one update after the hit that rolled it, outside the damage code.
    class HeartseedProcEvent : public BasicEvent
    {
    public:
        HeartseedProcEvent(Player* player, ObjectGuid victim) : _player(player), _victim(victim) { }

        bool Execute(uint64 /*time*/, uint32 /*diff*/) override
        {
            Unit* victim = ObjectAccessor::GetUnit(*_player, _victim);
            if (victim && victim->IsAlive() && _player->IsAlive() && _player->IsValidAttackTarget(victim))
                sHeartseedMgr->CastProc(_player, victim);
            return true;
        }

    private:
        Player* _player;
        ObjectGuid _victim;
    };
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
    }

    TC_LOG_INFO("server.loading", ">> Loaded %u Heartseed stages, %u glows and %u traits in %u ms", uint32(_stages.size()), uint32(_glow.size()),
        uint32(_traits.size()), GetMSTimeDiffToNow(oldMSTime));
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
    if (!_enabled || !_procSpellId || _procChance <= 0.0f || victim == player || InProc)
        return;

    uint32 now = getMSTime();
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _states.find(player->GetGUID().GetCounter());
        if (itr == _states.end())
            return;
        Heartseed::State& state = itr->second;
        if (state.HasProcced && getMSTimeDiff(state.LastProcTime, now) < _procCooldown)
            return;
        if (!roll_chance_f(_procChance))
            return;
        state.LastProcTime = now;
        state.HasProcced = true;
    }

    player->m_Events.AddEvent(new HeartseedProcEvent(player, victim->GetGUID()), player->m_Events.CalculateTime(0));
}

uint32 HeartseedMgr::GetProcDamage(Player* player) const
{
    Item* weapon = player->GetItemByPos(INVENTORY_SLOT_BAG_0, EQUIPMENT_SLOT_MAINHAND);
    WeaponAttackType attackType = weapon && weapon->GetTemplate()->IsRangedInventoryType() ? RANGED_ATTACK : BASE_ATTACK;
    return Heartseed::CoreProcDamage(player->GetWeaponDamageRange(attackType, MINDAMAGE), player->GetWeaponDamageRange(attackType, MAXDAMAGE),
        _procWeaponDamagePct);
}

void HeartseedMgr::CastProc(Player* player, Unit* victim)
{
    if (!_procSpellId || !sSpellMgr->GetSpellInfo(_procSpellId))
        return;
    int32 damage = int32(GetProcDamage(player));
    if (damage <= 0)
        return;

    // Dealt directly rather than cast: a cast checks range and facing even when triggered, which would drop
    // procs from ranged hits. The spell supplies the name, school and bonuses shown in the combat log.
    InProc = true;
    player->SpellNonMeleeDamageLog(victim, _procSpellId, uint32(damage));
    InProc = false;

    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _states.find(player->GetGUID().GetCounter());
    if (itr != _states.end())
        ++itr->second.ProcCount;
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
