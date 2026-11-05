/*
 * Multi-classing (T-094, docs/DESIGN.md "Multi-classing"): a base class takes on a second class that levels on its
 * own. Second classes, their abilities and their interaction rules are rows in the world database; a character's
 * second classes are rows in the characters database. See README.md in this folder.
 */

#ifndef ULTIMATEMOP_MULTICLASS_MGR_H
#define ULTIMATEMOP_MULTICLASS_MGR_H

#include "Define.h"
#include "MulticlassRules.h"
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Player;
class Unit;

namespace Multiclass
{
    struct ClassInfo
    {
        uint32 Id = 0;
        std::string Name;
        uint32 BaseClassMask = 0;   // 1 << (class id - 1)
        uint8 MaxLevel = 30;
        std::string Description;
    };

    enum class AbilityKind : uint8
    {
        Active  = 0,                // learned into the spellbook
        Passive = 1,                // works through rules; listed in the panel, never learned
    };

    struct AbilityInfo
    {
        uint32 ClassId = 0;
        uint8 Level = 1;
        uint32 SpellId = 0;
        AbilityKind Kind = AbilityKind::Active;
        int32 PowerPct = 0;         // damage per hit, % of the Gift power; 0 keeps the spell's own
        uint32 CooldownMs = 0;
        uint8 PowerType = 0;        // Powers
        uint32 CostPct = 0;         // % of the character's maximum of that power
        std::string Name;
        std::string Description;
    };

    enum class RuleTrigger : uint8
    {
        Cast = 1,                   // the character casts TriggerSpell (not triggered)
        Hit  = 2,                   // a second-class ability (TriggerSpell) hits a target
        Kill = 3,                   // the character deals a killing blow to a creature (TriggerSpell 0 = any)
    };

    enum class RuleCondition : uint8
    {
        None              = 0,
        TargetAura        = 1,      // the target has ConditionSpell from the character
        CasterAura        = 2,      // the character has ConditionSpell
        TargetNotOnCaster = 3,      // the target is attacking someone else
    };

    enum class RuleEffect : uint8
    {
        CastOnTarget = 1,           // EffectSpell, triggered, on the target
        CastOnSelf   = 2,           // EffectSpell, triggered, on the character
        Energize     = 3,           // EffectSpell is the power type, Amount the power
    };

    struct RuleInfo
    {
        uint32 Id = 0;
        uint32 ClassId = 0;
        uint32 BaseSpec = 0;        // ChrSpecialization id; 0 for any spec
        uint8 MinLevel = 1;         // second-class level
        RuleTrigger On = RuleTrigger::Cast;
        uint32 TriggerSpell = 0;
        RuleCondition If = RuleCondition::None;
        uint32 ConditionSpell = 0;
        RuleEffect Do = RuleEffect::CastOnTarget;
        uint32 EffectSpell = 0;
        int32 Amount = 0;
        uint32 CooldownMs = 0;
        std::string Comment;
    };

    struct CharacterState
    {
        std::map<uint32, Progress> Classes;
        uint32 Active = 0;
        std::map<uint32, uint32> RuleFiredAt;
        uint32 LastRule = 0;         // for .multiclass info
    };
}

class MulticlassMgr
{
public:
    static MulticlassMgr* instance();

    void LoadConfig();
    void LoadData();                // world tables; also `.multiclass reload`
    bool IsEnabled() const { return _enabled; }
    uint8 GetUnlockLevel() const { return _unlockLevel; }

    void OnLogin(Player* player);
    void OnLogout(Player* player);
    // Mirrors experience into the active second class.
    void GainXp(Player* player, uint32 amount);

    // Classes the character's base class can take, in id order.
    std::vector<Multiclass::ClassInfo> GetClassesFor(Player const* player) const;
    Multiclass::ClassInfo const* GetClass(uint32 classId) const;
    Multiclass::AbilityInfo const* GetAbility(uint32 spellId) const;
    std::vector<Multiclass::AbilityInfo> GetAbilities(uint32 classId) const;
    std::vector<Multiclass::RuleInfo> GetRules(uint32 classId) const;
    uint32 XpPerLevel(Player const* player) const;

    // The active second class and its progress; false when none.
    bool GetActive(Player const* player, uint32& classId, Multiclass::Progress& progress) const;
    std::map<uint32, Multiclass::Progress> GetProgress(Player const* player) const;
    uint32 GetLastRule(Player const* player) const;

    // Makes classId active (learning it at level 1 if new). False with a reason for the player.
    bool Switch(Player* player, uint32 classId, std::string& error, bool gm = false);
    // GM: sets a second class's level and experience (adds it if missing) and reapplies abilities.
    bool SetLevel(Player* player, uint32 classId, uint8 level, uint32 xp, std::string& error);
    void Clear(Player* player);

    // Rules engine: runs every rule of the character's active second class for this trigger.
    void Fire(Player* player, Multiclass::RuleTrigger on, uint32 spellId, Unit* target);

    void SendSync(Player* player) const;

private:
    void ApplyAbilities(Player* player) const;
    void Save(Player const* player) const;

    bool _enabled = true;
    uint8 _unlockLevel = 30;
    uint32 _xpPct = 25;

    mutable std::mutex _lock;
    std::map<uint32, Multiclass::ClassInfo> _classes;
    std::unordered_map<uint32, Multiclass::AbilityInfo> _abilities;                  // by spell id
    std::unordered_map<uint32, std::vector<Multiclass::RuleInfo>> _rulesBySpell;    // by trigger spell
    std::vector<Multiclass::RuleInfo> _killRules;
    std::unordered_map<uint32, Multiclass::CharacterState> _states;                 // by character guid
};

#define sMulticlassMgr MulticlassMgr::instance()

#endif
