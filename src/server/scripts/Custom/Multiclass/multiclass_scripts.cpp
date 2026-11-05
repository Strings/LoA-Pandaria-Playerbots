/*
 * Multi-classing (T-094): hooks, commands and the second-class ability spell scripts. See README.md.
 */

#include "MulticlassMgr.h"
#include "../Heartseed/HeartseedMgr.h"
#include "../Heartseed/HeartseedRules.h"
#include "../Reload/ReloadRegistry.h"
#include "Chat.h"
#include "Creature.h"
#include "DBCStores.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "QuestDef.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "World.h"
#include "WorldSession.h"
#include <algorithm>

using namespace Multiclass;

namespace
{
    // IDs from the multi-classing gossip block (docs/ID_RANGES.md).
    rbac::RBACPermissions const GmPermission = static_cast<rbac::RBACPermissions>(100000);      // .multiclass
    rbac::RBACPermissions const PlayerPermission = static_cast<rbac::RBACPermissions>(100001);  // .mc (the addon's)

    uint32 const SPELL_CORPSE_EXPLOSION = 320005;
}

class multiclass_playerscript : public PlayerScript
{
public:
    multiclass_playerscript() : PlayerScript("multiclass_playerscript") { }

    void OnLogin(Player* player) override { sMulticlassMgr->OnLogin(player); }
    void OnLogout(Player* player) override { sMulticlassMgr->OnLogout(player); }

    void OnGiveXP(Player* player, uint32& amount, Unit* /*victim*/) override
    {
        sMulticlassMgr->GainXp(player, amount);
    }

    // At the top level a quest pays money instead of calling GiveXP, so the second class gets the quest's
    // experience here.
    void OnQuestRewarded(Player* player, Quest const* quest) override
    {
        if (player->GetLevel() >= sWorld->getIntConfig(CONFIG_MAX_PLAYER_LEVEL))
            sMulticlassMgr->GainXp(player, quest->XPValue(player));
    }

    void OnLevelChanged(Player* player, uint8 oldLevel) override
    {
        uint8 unlock = sMulticlassMgr->GetUnlockLevel();
        if (sMulticlassMgr->IsEnabled() && oldLevel < unlock && player->GetLevel() >= unlock && !sMulticlassMgr->GetClassesFor(player).empty())
            ChatHandler(player->GetSession()).PSendSysMessage("|cff66ccffYour training opens a second path. Press Alt-N (or type /mc) to choose a second class.|r");
        sMulticlassMgr->SendSync(player);
    }
};

class multiclass_unitscript : public UnitScript
{
public:
    multiclass_unitscript() : UnitScript("multiclass_unitscript") { }

    // Cast rules: the character casts a spell on purpose (triggered casts never start rules, so rules can't chain).
    void OnUnitSpellCast(Spell* spell, Unit* caster, SpellInfo const* spellInfo, bool /*skipCheck*/) override
    {
        Player* player = caster ? caster->ToPlayer() : nullptr;
        if (!player || !spell || spell->IsTriggered())
            return;
        sMulticlassMgr->Fire(player, RuleTrigger::Cast, spellInfo->Id, spell->m_targets.GetUnitTarget());
    }

    // Kill rules run on the killing blow, before death clears the victim's auras, so conditions can read them.
    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!attacker || !victim || victim->GetTypeId() != TYPEID_UNIT || !victim->IsAlive() || damage < victim->GetHealth())
            return;
        Player* player = attacker->GetCharmerOrOwnerPlayerOrPlayerItself();
        if (player && player->IsInWorld())
            sMulticlassMgr->Fire(player, RuleTrigger::Kill, 0, victim);
    }
};

class multiclass_worldscript : public WorldScript
{
public:
    multiclass_worldscript() : WorldScript("multiclass_worldscript") { }

    void OnConfigLoad(bool /*reload*/) override { sMulticlassMgr->LoadConfig(); }
    void OnStartup() override { sMulticlassMgr->LoadData(); }
};

class multiclass_commandscript : public CommandScript
{
public:
    multiclass_commandscript() : CommandScript("multiclass_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> gmTable =
        {
            { "info",   &HandleInfo,   GmPermission, Trinity::ChatCommands::Console::No },
            { "set",    &HandleSet,    GmPermission, Trinity::ChatCommands::Console::No },
            { "xp",     &HandleXp,     GmPermission, Trinity::ChatCommands::Console::No },
            { "switch", &HandleGmSwitch, GmPermission, Trinity::ChatCommands::Console::No },
            { "clear",  &HandleClear,  GmPermission, Trinity::ChatCommands::Console::No },
            { "rules",  &HandleRules,  GmPermission, Trinity::ChatCommands::Console::Yes },
            { "spec",   &HandleSpec,   GmPermission, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> playerTable =
        {
            { "sync",   &HandleSync,   PlayerPermission, Trinity::ChatCommands::Console::No },
            { "switch", &HandleSwitch, PlayerPermission, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "multiclass", gmTable,     GmPermission,     Trinity::ChatCommands::Console::Yes },
            { "mc",         playerTable, PlayerPermission, Trinity::ChatCommands::Console::No },
        };
        return commandTable;
    }

    // The selected player, or yourself.
    static Player* Target(ChatHandler* handler)
    {
        Player* target = handler->getSelectedPlayer();
        return target ? target : handler->GetSession()->GetPlayer();
    }

    // A second class by id or by name (case-insensitive prefix) among those the player's base class can take.
    static uint32 FindClass(Player const* player, std::string const& arg)
    {
        if (arg.empty())
            return 0;
        if (std::all_of(arg.begin(), arg.end(), ::isdigit))
            return uint32(std::stoul(arg));
        std::string want = arg;
        std::transform(want.begin(), want.end(), want.begin(), ::tolower);
        for (ClassInfo const& info : sMulticlassMgr->GetClassesFor(player))
        {
            std::string name = info.Name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name.compare(0, want.size(), want) == 0)
                return info.Id;
        }
        return 0;
    }

    static std::vector<std::string> Words(char const* args)
    {
        std::vector<std::string> words;
        std::string word;
        for (char const* c = args ? args : ""; *c; ++c)
        {
            if (*c == ' ')
            {
                if (!word.empty())
                    words.push_back(word);
                word.clear();
            }
            else
                word += *c;
        }
        if (!word.empty())
            words.push_back(word);
        return words;
    }

    static bool HandleSync(ChatHandler* handler, char const* /*args*/)
    {
        sMulticlassMgr->SendSync(handler->GetSession()->GetPlayer());
        return true;
    }

    // .mc switch <class name or id | none>
    static bool HandleSwitch(ChatHandler* handler, char const* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        std::vector<std::string> words = Words(args);
        if (words.empty())
        {
            handler->SendSysMessage("Usage: .mc switch <second class | none>");
            return true;
        }
        uint32 classId = words[0] == "none" ? 0 : FindClass(player, words[0]);
        if (!classId && words[0] != "none")
        {
            handler->PSendSysMessage("No second class '%s' for your class.", words[0].c_str());
            return true;
        }
        std::string error;
        if (!sMulticlassMgr->Switch(player, classId, error))
            handler->SendSysMessage(error.c_str());
        else if (ClassInfo const* info = classId ? sMulticlassMgr->GetClass(classId) : nullptr)
            handler->PSendSysMessage("You walk the path of the %s.", info->Name.c_str());
        else
            handler->SendSysMessage("You set your second class aside.");
        return true;
    }

    static bool HandleGmSwitch(ChatHandler* handler, char const* args)
    {
        Player* target = Target(handler);
        std::vector<std::string> words = Words(args);
        uint32 classId = words.empty() || words[0] == "none" ? 0 : FindClass(target, words[0]);
        std::string error;
        if (!sMulticlassMgr->Switch(target, classId, error, true))
            handler->SendSysMessage(error.c_str());
        else
            handler->PSendSysMessage("%s: active second class %u.", target->GetName().c_str(), classId);
        return true;
    }

    // .multiclass set <class> <level> [xp]
    static bool HandleSet(ChatHandler* handler, char const* args)
    {
        Player* target = Target(handler);
        std::vector<std::string> words = Words(args);
        uint32 classId = words.size() >= 2 ? FindClass(target, words[0]) : 0;
        if (!classId || !std::all_of(words[1].begin(), words[1].end(), ::isdigit))
        {
            handler->SendSysMessage("Usage: .multiclass set <second class> <level> [xp]");
            return true;
        }
        uint32 xp = words.size() >= 3 ? uint32(std::stoul(words[2])) : 0;
        std::string error;
        if (!sMulticlassMgr->SetLevel(target, classId, uint8(std::min<unsigned long>(std::stoul(words[1]), 255)), xp, error))
            handler->SendSysMessage(error.c_str());
        else
            HandleInfo(handler, "");
        return true;
    }

    // .multiclass xp <amount>: experience into the active second class, as if earned.
    static bool HandleXp(ChatHandler* handler, char const* args)
    {
        Player* target = Target(handler);
        std::vector<std::string> words = Words(args);
        if (words.empty() || !std::all_of(words[0].begin(), words[0].end(), ::isdigit))
        {
            handler->SendSysMessage("Usage: .multiclass xp <amount>");
            return true;
        }
        sMulticlassMgr->GainXp(target, uint32(std::stoul(words[0])));
        HandleInfo(handler, "");
        return true;
    }

    static bool HandleClear(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = Target(handler);
        sMulticlassMgr->Clear(target);
        handler->PSendSysMessage("%s: second classes cleared.", target->GetName().c_str());
        return true;
    }

    // .multiclass info: the target's second classes, levels, learned abilities and live rules.
    static bool HandleInfo(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = Target(handler);
        uint32 active = 0;
        Progress activeProgress;
        sMulticlassMgr->GetActive(target, active, activeProgress);
        std::map<uint32, Progress> progress = sMulticlassMgr->GetProgress(target);
        handler->PSendSysMessage("%s (class %u, spec %u, level %u): active second class %u; %u per second-class level; opens at %u.",
            target->GetName().c_str(), uint32(target->GetClass()), target->GetTalentSpecialization(), uint32(target->GetLevel()), active,
            sMulticlassMgr->XpPerLevel(target), uint32(sMulticlassMgr->GetUnlockLevel()));
        for (ClassInfo const& info : sMulticlassMgr->GetClassesFor(target))
        {
            auto itr = progress.find(info.Id);
            if (itr == progress.end())
            {
                handler->PSendSysMessage("  %u %s: not taken.", info.Id, info.Name.c_str());
                continue;
            }
            std::string learned;
            for (AbilityInfo const& ability : sMulticlassMgr->GetAbilities(info.Id))
                if (ability.Level <= itr->second.Level)
                    learned += Trinity::StringFormat("%s%s %u%s", learned.empty() ? "" : ", ", ability.Name.c_str(), ability.SpellId,
                        ability.Kind == AbilityKind::Passive ? " (passive)" : target->HasSpell(ability.SpellId) ? "" : " (not learned)");
            handler->PSendSysMessage("  %u %s%s: level %u/%u, xp %u. Abilities: %s.", info.Id, info.Name.c_str(), info.Id == active ? " (active)" : "",
                uint32(itr->second.Level), uint32(info.MaxLevel), itr->second.Xp, learned.empty() ? "none" : learned.c_str());
        }
        if (uint32 last = sMulticlassMgr->GetLastRule(target))
            handler->PSendSysMessage("  Last interaction: rule %u.", last);
        return true;
    }

    // .multiclass spec <0-2>: picks the target's specialization the way the client's spec choice does (the target
    // must have none; .respec reset spec clears it), so per-spec interactions can be tested without the client UI.
    static bool HandleSpec(ChatHandler* handler, char const* args)
    {
        Player* target = Target(handler);
        std::vector<std::string> words = Words(args);
        if (words.empty() || words[0].size() != 1 || words[0][0] < '0' || words[0][0] > '2')
        {
            handler->SendSysMessage("Usage: .multiclass spec <0|1|2> (the class's spec tab)");
            return true;
        }
        if (target->GetTalentSpecialization(target->GetActiveSpec()))
        {
            handler->PSendSysMessage("%s already has spec %u; clear it with .respec reset spec.", target->GetName().c_str(),
                target->GetTalentSpecialization());
            return true;
        }
        uint32 specializationId = dbc::GetClassSpecializations(target->GetClass())[words[0][0] - '0'];
        target->SetTalentSpecialization(target->GetActiveSpec(), Specializations(specializationId));
        target->SetUInt32Value(PLAYER_FIELD_CURRENT_SPEC_ID, specializationId);
        target->SendTalentsInfoData();
        target->LearnSpecializationSpells();
        target->UpdatePvpPower();
        sScriptMgr->OnPlayerSpecializationChanged(target);
        handler->PSendSysMessage("%s: spec %u.", target->GetName().c_str(), specializationId);
        return true;
    }

    // .multiclass rules [class]: the interaction rules, for checking what a pairing does.
    static bool HandleRules(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> words = Words(args);
        uint32 classId = words.empty() ? 0 : uint32(std::strtoul(words[0].c_str(), nullptr, 10));
        std::vector<RuleInfo> rules = sMulticlassMgr->GetRules(classId);
        handler->PSendSysMessage("%u interaction rules.", uint32(rules.size()));
        for (RuleInfo const& rule : rules)
            handler->PSendSysMessage("  %u class %u spec %u lvl %u: trigger %u spell %u, if %u %u, do %u %u (%d), cd %u: %s", rule.Id, rule.ClassId,
                rule.BaseSpec, uint32(rule.MinLevel), uint32(rule.On), rule.TriggerSpell, uint32(rule.If), rule.ConditionSpell, uint32(rule.Do),
                rule.EffectSpell, rule.Amount, rule.CooldownMs, rule.Comment.c_str());
        return true;
    }
};

// Every second-class ability: its cost (a share of the character's maximum power) and cooldown from
// multiclass_ability, its damage as a share of the Gift power (level base plus weighted attack or spell power, so it
// scales like the Gifts), and Hit rules for the interactions.
class spell_multiclass_ability : public SpellScript
{
    PrepareSpellScript(spell_multiclass_ability);

    bool Load() override
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        AbilityInfo const* ability = sMulticlassMgr->GetAbility(GetSpellInfo()->Id);
        if (!player || !ability)
            return false;
        _ability = *ability;
        return true;
    }

    uint32 Cost()
    {
        Player* player = GetCaster()->ToPlayer();
        return uint32(uint64(player->GetMaxPower(Powers(_ability.PowerType))) * _ability.CostPct / 100);
    }

    SpellCastResult CheckCost()
    {
        if (GetSpell()->IsTriggered() || !_ability.CostPct)
            return SPELL_CAST_OK;
        Player* player = GetCaster()->ToPlayer();
        return uint32(std::max(0, player->GetPower(Powers(_ability.PowerType)))) >= Cost() ? SPELL_CAST_OK : SPELL_FAILED_NO_POWER;
    }

    void PayAndCool()
    {
        if (GetSpell()->IsTriggered())
            return;
        Player* player = GetCaster()->ToPlayer();
        if (_ability.CostPct)
            player->ModifyPower(Powers(_ability.PowerType), -int32(Cost()));
        if (_ability.CooldownMs)
        {
            player->RemoveSpellCooldown(_ability.SpellId, true);
            player->AddCustomCooldown(_ability.SpellId, Milliseconds(_ability.CooldownMs));
        }
    }

    void HandleDamage(SpellEffIndex effIndex)
    {
        if (_ability.PowerPct <= 0)
            return;
        switch (GetSpellInfo()->Effects[effIndex].Effect)
        {
            case SPELL_EFFECT_SCHOOL_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE:
            case SPELL_EFFECT_WEAPON_DAMAGE_NOSCHOOL:
            case SPELL_EFFECT_WEAPON_PERCENT_DAMAGE:
            case SPELL_EFFECT_NORMALIZED_WEAPON_DMG:
                break;
            default:
                return;
        }
        PreventHitDefaultEffect(effIndex);
        Player* player = GetCaster()->ToPlayer();
        SetHitDamage(int32(std::min<uint32>(Heartseed::PercentOf(sHeartseedMgr->GetGiftPower(player), _ability.PowerPct), INT32_MAX)));
    }

    // Some source spells (Bone Spike for Bone Spear) knock the target back; a second-class ability never does.
    void HandleNoKnockback(SpellEffIndex effIndex)
    {
        PreventHitDefaultEffect(effIndex);
    }

    void HandleHitRules()
    {
        if (GetSpell()->IsTriggered())
            return;
        sMulticlassMgr->Fire(GetCaster()->ToPlayer(), RuleTrigger::Hit, GetSpellInfo()->Id, GetHitUnit());
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_multiclass_ability::CheckCost);
        AfterCast += SpellCastFn(spell_multiclass_ability::PayAndCool);
        OnEffectHitTarget += SpellEffectFn(spell_multiclass_ability::HandleDamage, EFFECT_0, SPELL_EFFECT_ANY);
        OnEffectHitTarget += SpellEffectFn(spell_multiclass_ability::HandleNoKnockback, EFFECT_ALL, SPELL_EFFECT_KNOCK_BACK);
        AfterHit += SpellHitFn(spell_multiclass_ability::HandleHitRules);
    }

    AbilityInfo _ability;
};

namespace
{
    // A creature corpse in range that has not been used up yet.
    struct CorpseCheck
    {
        CorpseCheck(Unit const* caster, float range) : _caster(caster), _range(range) { }
        bool operator()(Creature* creature) const
        {
            return !creature->IsAlive() && creature->GetCreatureType() != CREATURE_TYPE_CRITTER && !creature->IsSummon()
                && _caster->IsWithinDistInMap(creature, _range) && _caster->IsWithinLOSInMap(creature);
        }
        Unit const* _caster;
        float _range;
    };

    Creature* NearestCorpse(Unit* caster, float range)
    {
        std::list<Creature*> corpses;
        CorpseCheck check(caster, range);
        Trinity::CreatureListSearcher<CorpseCheck> searcher(caster, corpses, check);
        caster->VisitNearbyGridObject(range, searcher);
        Creature* nearest = nullptr;
        for (Creature* corpse : corpses)
            if (!nearest || caster->GetExactDist2d(corpse) < caster->GetExactDist2d(nearest))
                nearest = corpse;
        return nearest;
    }
}

// 320005 - Corpse Explosion (Necromancer; a clone of 61614). Needs a corpse within 30 yd: it bursts, hitting enemies
// within 8 yd of it, and is used up. The source spell also transforms its caster; that effect is dropped.
class spell_multiclass_corpse_explosion : public SpellScript
{
    PrepareSpellScript(spell_multiclass_corpse_explosion);

    SpellCastResult CheckCorpse()
    {
        Creature* corpse = NearestCorpse(GetCaster(), 30.0f);
        if (!corpse)
        {
            SetCustomCastResultMessage(SPELL_CUSTOM_ERROR_NO_NEARBY_CORPSES);
            return SPELL_FAILED_CUSTOM_ERROR;
        }
        _corpse = corpse->GetGUID();
        return SPELL_CAST_OK;
    }

    void SelectEnemies(std::list<WorldObject*>& targets)
    {
        targets.clear();
        Unit* caster = GetCaster();
        Creature* corpse = ObjectAccessor::GetCreature(*caster, _corpse);
        if (!corpse)
            return;
        std::list<Unit*> nearby;
        corpse->GetAttackableUnitListInRange(nearby, 8.0f);
        for (Unit* unit : nearby)
            if (unit->IsAlive() && caster->IsValidAttackTarget(unit))
                targets.push_back(unit);
    }

    void DropTransform(WorldObject*& target)
    {
        target = nullptr;
    }

    void UseUpCorpse()
    {
        if (Creature* corpse = ObjectAccessor::GetCreature(*GetCaster(), _corpse))
            corpse->DespawnOrUnsummon(500);
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_multiclass_corpse_explosion::CheckCorpse);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_multiclass_corpse_explosion::SelectEnemies, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
        OnObjectTargetSelect += SpellObjectTargetSelectFn(spell_multiclass_corpse_explosion::DropTransform, EFFECT_1, TARGET_UNIT_CASTER);
        AfterCast += SpellCastFn(spell_multiclass_corpse_explosion::UseUpCorpse);
    }

    ObjectGuid _corpse;
};

void AddSC_multiclass()
{
    new multiclass_playerscript();
    new multiclass_unitscript();
    new multiclass_worldscript();
    new multiclass_commandscript();
    RegisterSpellScript(spell_multiclass_ability);
    RegisterSpellScript(spell_multiclass_corpse_explosion);
    CustomReload::Tables().Register("multiclass", "Multi-classing: second classes, abilities and interaction rules", []
    {
        sMulticlassMgr->LoadData();
        return CustomReload::Result{ true, 0, "multi-classing reloaded" };
    });
}
