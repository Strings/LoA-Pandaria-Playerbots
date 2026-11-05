/*
 * Multi-classing manager (T-094). See MulticlassMgr.h and README.md.
 */

#include "MulticlassMgr.h"
#include "Chat.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "SpellMgr.h"
#include "StringFormat.h"
#include "Timer.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"
#include <algorithm>

using namespace Multiclass;

MulticlassMgr* MulticlassMgr::instance()
{
    static MulticlassMgr instance;
    return &instance;
}

void MulticlassMgr::LoadConfig()
{
    _enabled = sConfigMgr->GetBoolDefault("Multiclass.Enable", true);
    _unlockLevel = uint8(std::clamp(sConfigMgr->GetIntDefault("Multiclass.UnlockLevel", 30), 1, 255));
    _xpPct = uint32(std::max(1, sConfigMgr->GetIntDefault("Multiclass.XpPct", 25)));
}

void MulticlassMgr::LoadData()
{
    std::map<uint32, ClassInfo> classes;
    std::unordered_map<uint32, AbilityInfo> abilities;
    std::unordered_map<uint32, std::vector<RuleInfo>> rulesBySpell;
    std::vector<RuleInfo> killRules;

    if (QueryResult result = WorldDatabase.Query("SELECT `id`, `name`, `base_class_mask`, `max_level`, `description` FROM `multiclass_class`"))
    {
        do
        {
            Field* fields = result->Fetch();
            ClassInfo info;
            info.Id = fields[0].GetUInt32();
            info.Name = fields[1].GetString();
            info.BaseClassMask = fields[2].GetUInt32();
            info.MaxLevel = std::max<uint8>(fields[3].GetUInt8(), 1);
            info.Description = fields[4].GetString();
            classes[info.Id] = info;
        } while (result->NextRow());
    }

    if (QueryResult result = WorldDatabase.Query("SELECT `class_id`, `level`, `spell_id`, `kind`, `power_pct`, `cooldown`, `power_type`, "
        "`cost_pct`, `name`, `description` FROM `multiclass_ability`"))
    {
        do
        {
            Field* fields = result->Fetch();
            AbilityInfo info;
            info.ClassId = fields[0].GetUInt32();
            info.Level = fields[1].GetUInt8();
            info.SpellId = fields[2].GetUInt32();
            info.Kind = AbilityKind(fields[3].GetUInt8());
            info.PowerPct = fields[4].GetInt32();
            info.CooldownMs = fields[5].GetUInt32();
            info.PowerType = fields[6].GetUInt8();
            info.CostPct = fields[7].GetUInt32();
            info.Name = fields[8].GetString();
            info.Description = fields[9].GetString();
            if (!classes.count(info.ClassId) || !sSpellMgr->GetSpellInfo(info.SpellId) || info.PowerType >= MAX_POWERS)
            {
                TC_LOG_ERROR("sql.sql", "multiclass_ability: spell %u (class %u) skipped: unknown class or spell, or bad power type.",
                    info.SpellId, info.ClassId);
                continue;
            }
            abilities[info.SpellId] = info;
        } while (result->NextRow());
    }

    if (QueryResult result = WorldDatabase.Query("SELECT `id`, `class_id`, `base_spec`, `min_level`, `trigger`, `trigger_spell`, `condition`, "
        "`condition_spell`, `effect`, `effect_spell`, `amount`, `cooldown`, `comment` FROM `multiclass_rule`"))
    {
        do
        {
            Field* fields = result->Fetch();
            RuleInfo rule;
            rule.Id = fields[0].GetUInt32();
            rule.ClassId = fields[1].GetUInt32();
            rule.BaseSpec = fields[2].GetUInt32();
            rule.MinLevel = fields[3].GetUInt8();
            rule.On = Trigger(fields[4].GetUInt8());
            rule.TriggerSpell = fields[5].GetUInt32();
            rule.If = Condition(fields[6].GetUInt8());
            rule.ConditionSpell = fields[7].GetUInt32();
            rule.Do = Effect(fields[8].GetUInt8());
            rule.EffectSpell = fields[9].GetUInt32();
            rule.Amount = fields[10].GetInt32();
            rule.CooldownMs = fields[11].GetUInt32();
            rule.Comment = fields[12].GetString();
            bool castsSpell = rule.Do == Effect::CastOnTarget || rule.Do == Effect::CastOnSelf;
            if (!classes.count(rule.ClassId) || (castsSpell && !sSpellMgr->GetSpellInfo(rule.EffectSpell))
                || rule.On < Trigger::Cast || rule.On > Trigger::Kill || rule.Do < Effect::CastOnTarget || rule.Do > Effect::Energize)
            {
                TC_LOG_ERROR("sql.sql", "multiclass_rule %u skipped: unknown class, trigger, effect or effect spell.", rule.Id);
                continue;
            }
            if (rule.On == Trigger::Kill)
                killRules.push_back(rule);
            else
                rulesBySpell[rule.TriggerSpell].push_back(rule);
        } while (result->NextRow());
    }

    std::lock_guard<std::mutex> guard(_lock);
    TC_LOG_INFO("server.loading", ">> Loaded %u second classes, %u abilities and %u interaction rules (multi-classing)",
        uint32(classes.size()), uint32(abilities.size()), uint32(killRules.size() + [&] { size_t n = 0; for (auto const& r : rulesBySpell) n += r.second.size(); return n; }()));
    _classes = std::move(classes);
    _abilities = std::move(abilities);
    _rulesBySpell = std::move(rulesBySpell);
    _killRules = std::move(killRules);
}

void MulticlassMgr::OnLogin(Player* player)
{
    if (!_enabled)
        return;
    CharacterState state;
    if (QueryResult result = CharacterDatabase.PQuery("SELECT `class_id`, `level`, `xp`, `active` FROM `character_multiclass` WHERE `guid` = %u",
        player->GetGUID().GetCounter()))
    {
        do
        {
            Field* fields = result->Fetch();
            uint32 classId = fields[0].GetUInt32();
            state.Classes[classId] = { std::max<uint8>(fields[1].GetUInt8(), 1), fields[2].GetUInt32() };
            if (fields[3].GetUInt8())
                state.Active = classId;
        } while (result->NextRow());
    }
    {
        std::lock_guard<std::mutex> guard(_lock);
        _states[player->GetGUID().GetCounter()] = std::move(state);
    }
    ApplyAbilities(player);
}

void MulticlassMgr::OnLogout(Player* player)
{
    std::lock_guard<std::mutex> guard(_lock);
    _states.erase(player->GetGUID().GetCounter());
}

uint32 MulticlassMgr::XpPerLevel(Player const* player) const
{
    uint32 maxLevel = sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL);
    uint8 level = uint8(std::clamp<uint32>(player->GetLevel(), 1, maxLevel > 1 ? maxLevel - 1 : 1));
    return Multiclass::XpPerLevel(sObjectMgr->GetXPForLevel(level), _xpPct);
}

void MulticlassMgr::GainXp(Player* player, uint32 amount)
{
    if (!_enabled || !amount)
        return;
    uint32 perLevel = XpPerLevel(player);
    uint8 before = 0, after = 0;
    std::string className;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto state = _states.find(player->GetGUID().GetCounter());
        if (state == _states.end() || !state->second.Active)
            return;
        auto cls = _classes.find(state->second.Active);
        if (cls == _classes.end())
            return;
        Progress& progress = state->second.Classes[state->second.Active];
        before = progress.Level;
        progress = AddXp(progress, amount, perLevel, cls->second.MaxLevel);
        after = progress.Level;
        className = cls->second.Name;
    }
    if (after > before)
    {
        ApplyAbilities(player);
        ChatHandler(player->GetSession()).PSendSysMessage("Your %s reaches level %u.", className.c_str(), uint32(after));
    }
    Save(player);
    SendSync(player);
}

std::vector<ClassInfo> MulticlassMgr::GetClassesFor(Player const* player) const
{
    std::lock_guard<std::mutex> guard(_lock);
    std::vector<ClassInfo> out;
    uint32 mask = 1u << (player->GetClass() - 1);
    for (auto const& [id, info] : _classes)
        if (info.BaseClassMask & mask)
            out.push_back(info);
    return out;
}

ClassInfo const* MulticlassMgr::GetClass(uint32 classId) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _classes.find(classId);
    return itr != _classes.end() ? &itr->second : nullptr;
}

AbilityInfo const* MulticlassMgr::GetAbility(uint32 spellId) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _abilities.find(spellId);
    return itr != _abilities.end() ? &itr->second : nullptr;
}

std::vector<AbilityInfo> MulticlassMgr::GetAbilities(uint32 classId) const
{
    std::lock_guard<std::mutex> guard(_lock);
    std::vector<AbilityInfo> out;
    for (auto const& [spellId, info] : _abilities)
        if (info.ClassId == classId)
            out.push_back(info);
    std::sort(out.begin(), out.end(), [](AbilityInfo const& a, AbilityInfo const& b)
        { return a.Level != b.Level ? a.Level < b.Level : a.SpellId < b.SpellId; });
    return out;
}

std::vector<RuleInfo> MulticlassMgr::GetRules(uint32 classId) const
{
    std::lock_guard<std::mutex> guard(_lock);
    std::vector<RuleInfo> out;
    for (auto const& [spellId, rules] : _rulesBySpell)
        for (RuleInfo const& rule : rules)
            if (!classId || rule.ClassId == classId)
                out.push_back(rule);
    for (RuleInfo const& rule : _killRules)
        if (!classId || rule.ClassId == classId)
            out.push_back(rule);
    std::sort(out.begin(), out.end(), [](RuleInfo const& a, RuleInfo const& b) { return a.Id < b.Id; });
    return out;
}

bool MulticlassMgr::GetActive(Player const* player, uint32& classId, Progress& progress) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto state = _states.find(player->GetGUID().GetCounter());
    if (state == _states.end() || !state->second.Active)
        return false;
    classId = state->second.Active;
    auto itr = state->second.Classes.find(classId);
    progress = itr != state->second.Classes.end() ? itr->second : Progress{};
    return true;
}

std::map<uint32, Progress> MulticlassMgr::GetProgress(Player const* player) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto state = _states.find(player->GetGUID().GetCounter());
    return state != _states.end() ? state->second.Classes : std::map<uint32, Progress>{};
}

bool MulticlassMgr::Switch(Player* player, uint32 classId, std::string& error, bool gm)
{
    if (!_enabled)
    {
        error = "Multi-classing is turned off.";
        return false;
    }
    if (classId)
    {
        ClassInfo const* info = GetClass(classId);
        if (!info || !(info->BaseClassMask & (1u << (player->GetClass() - 1))))
        {
            error = "Your class cannot take that second class.";
            return false;
        }
        if (!gm && player->GetLevel() < _unlockLevel)
        {
            error = Trinity::StringFormat("Second classes open at level %u.", uint32(_unlockLevel));
            return false;
        }
    }
    if (!gm && player->IsInCombat())
    {
        error = "You can't change your second class in combat.";
        return false;
    }
    {
        std::lock_guard<std::mutex> guard(_lock);
        CharacterState& state = _states[player->GetGUID().GetCounter()];
        if (classId && !state.Classes.count(classId))
            state.Classes[classId] = Progress{};
        state.Active = classId;
        state.RuleFiredAt.clear();
    }
    ApplyAbilities(player);
    Save(player);
    SendSync(player);
    return true;
}

bool MulticlassMgr::SetLevel(Player* player, uint32 classId, uint8 level, uint32 xp, std::string& error)
{
    ClassInfo const* info = GetClass(classId);
    if (!info)
    {
        error = "No such second class.";
        return false;
    }
    {
        std::lock_guard<std::mutex> guard(_lock);
        CharacterState& state = _states[player->GetGUID().GetCounter()];
        Progress& progress = state.Classes[classId];
        progress.Level = uint8(std::clamp<uint32>(level, 1, info->MaxLevel));
        progress.Xp = progress.Level >= info->MaxLevel ? 0 : xp;
    }
    ApplyAbilities(player);
    Save(player);
    SendSync(player);
    return true;
}

void MulticlassMgr::Clear(Player* player)
{
    {
        std::lock_guard<std::mutex> guard(_lock);
        _states[player->GetGUID().GetCounter()] = CharacterState{};
    }
    ApplyAbilities(player);
    CharacterDatabase.PExecute("DELETE FROM `character_multiclass` WHERE `guid` = %u", player->GetGUID().GetCounter());
    SendSync(player);
}

// Learns the active second class's abilities up to its level and unlearns every other second-class ability, so the
// spellbook always matches the active class (also repairs it at login).
void MulticlassMgr::ApplyAbilities(Player* player) const
{
    std::vector<uint32> learn, unlearn;
    {
        std::lock_guard<std::mutex> guard(_lock);
        uint32 active = 0;
        Progress progress;
        auto state = _states.find(player->GetGUID().GetCounter());
        if (state != _states.end() && state->second.Active)
        {
            active = state->second.Active;
            if (auto itr = state->second.Classes.find(active); itr != state->second.Classes.end())
                progress = itr->second;
        }
        for (auto const& [spellId, info] : _abilities)
        {
            if (info.Kind != AbilityKind::Active)
                continue;
            if (_enabled && info.ClassId == active && info.Level <= progress.Level)
                learn.push_back(spellId);
            else
                unlearn.push_back(spellId);
        }
    }
    for (uint32 spellId : unlearn)
        if (player->HasSpell(spellId))
            player->RemoveSpell(spellId, false, false);
    for (uint32 spellId : learn)
        if (!player->HasSpell(spellId))
            player->LearnSpell(spellId, false);
}

void MulticlassMgr::Save(Player const* player) const
{
    uint32 guid = player->GetGUID().GetCounter();
    std::map<uint32, Progress> classes;
    uint32 active = 0;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto state = _states.find(guid);
        if (state == _states.end())
            return;
        classes = state->second.Classes;
        active = state->second.Active;
    }
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    trans->PAppend("DELETE FROM `character_multiclass` WHERE `guid` = %u", guid);
    for (auto const& [classId, progress] : classes)
        trans->PAppend("INSERT INTO `character_multiclass` (`guid`, `class_id`, `level`, `xp`, `active`) VALUES (%u, %u, %u, %u, %u)",
            guid, classId, uint32(progress.Level), progress.Xp, classId == active ? 1u : 0u);
    CharacterDatabase.CommitTransaction(trans);
}

void MulticlassMgr::Fire(Player* player, Trigger on, uint32 spellId, Unit* target)
{
    if (!_enabled)
        return;
    std::vector<RuleInfo> ready;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto state = _states.find(player->GetGUID().GetCounter());
        if (state == _states.end() || !state->second.Active)
            return;
        uint32 active = state->second.Active;
        uint8 level = 1;
        if (auto itr = state->second.Classes.find(active); itr != state->second.Classes.end())
            level = itr->second.Level;

        std::vector<RuleInfo> const* rules = nullptr;
        if (on == Trigger::Kill)
            rules = &_killRules;
        else if (auto itr = _rulesBySpell.find(spellId); itr != _rulesBySpell.end())
            rules = &itr->second;
        if (!rules)
            return;

        uint32 now = getMSTime();
        uint32 spec = player->GetTalentSpecialization();
        for (RuleInfo const& rule : *rules)
        {
            if (rule.On != on || rule.ClassId != active || level < rule.MinLevel || (rule.BaseSpec && rule.BaseSpec != spec))
                continue;
            if (on == Trigger::Kill && rule.TriggerSpell && rule.TriggerSpell != spellId)
                continue;
            uint32& firedAt = state->second.RuleFiredAt[rule.Id];
            if (!RuleReady(firedAt, now, rule.CooldownMs))
                continue;
            ready.push_back(rule);
        }
    }

    for (RuleInfo const& rule : ready)
    {
        switch (rule.If)
        {
            case Condition::TargetAura:
                if (!target || !target->HasAura(rule.ConditionSpell, player->GetGUID()))
                    continue;
                break;
            case Condition::CasterAura:
                if (!player->HasAura(rule.ConditionSpell))
                    continue;
                break;
            case Condition::TargetNotOnCaster:
                if (!target || target->GetVictim() == player)
                    continue;
                break;
            default:
                break;
        }

        switch (rule.Do)
        {
            case Effect::CastOnTarget:
                if (!target || !target->IsAlive())
                    continue;
                player->CastSpell(target, rule.EffectSpell, TRIGGERED_FULL_MASK);
                break;
            case Effect::CastOnSelf:
                player->CastSpell(player, rule.EffectSpell, TRIGGERED_FULL_MASK);
                break;
            case Effect::Energize:
                if (rule.EffectSpell < MAX_POWERS)
                    player->ModifyPower(Powers(rule.EffectSpell), rule.Amount);
                break;
        }

        std::lock_guard<std::mutex> guard(_lock);
        auto state = _states.find(player->GetGUID().GetCounter());
        if (state != _states.end())
            state->second.RuleFiredAt[rule.Id] = std::max<uint32>(getMSTime(), 1);
    }
}

// Answers the Multiclass addon: whisper addon messages on prefix "Multiclass", tab separated, ending with "E".
//   U <unlock level> <character level> <xp per second-class level>
//   C <id> <name> <level> <xp> <max level> <active 0/1> <learned 0/1> <description>
//   A <class id> <level> <spell id> <kind> <cooldown ms> <name> <description>
void MulticlassMgr::SendSync(Player* player) const
{
    if (!player->GetSession())
        return;
    auto send = [player](std::string message)
    {
        if (message.size() > 250)
            message.resize(250);
        WorldPacket data;
        ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, player, player, message, 0, "", DEFAULT_LOCALE, "Multiclass");
        player->SendDirectMessage(&data);
    };

    send(Trinity::StringFormat("U\t%u\t%u\t%u", uint32(_unlockLevel), uint32(player->GetLevel()), XpPerLevel(player)));
    std::map<uint32, Progress> progress = GetProgress(player);
    uint32 active = 0;
    Progress unused;
    GetActive(player, active, unused);
    for (ClassInfo const& info : GetClassesFor(player))
    {
        auto itr = progress.find(info.Id);
        Progress p = itr != progress.end() ? itr->second : Progress{};
        send(Trinity::StringFormat("C\t%u\t%s\t%u\t%u\t%u\t%u\t%u\t%s", info.Id, info.Name.c_str(), uint32(p.Level), p.Xp,
            uint32(info.MaxLevel), info.Id == active ? 1u : 0u, itr != progress.end() ? 1u : 0u, info.Description.c_str()));
        for (AbilityInfo const& ability : GetAbilities(info.Id))
            send(Trinity::StringFormat("A\t%u\t%u\t%u\t%u\t%u\t%s\t%s", ability.ClassId, uint32(ability.Level), ability.SpellId,
                uint32(ability.Kind), ability.CooldownMs, ability.Name.c_str(), ability.Description.c_str()));
    }
    send("E");
}
