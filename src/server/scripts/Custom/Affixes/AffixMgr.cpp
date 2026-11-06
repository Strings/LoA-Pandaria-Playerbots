#include "AffixMgr.h"
#include "Containers.h"
#include "Creature.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Player.h"
#include "SpellAuraEffects.h"
#include "SpellScript.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"
#include <cctype>
#include <cmath>
#include <list>

namespace
{
    constexpr float JUGGERNAUT_SCALE = 1.25f;
    constexpr float JUGGERNAUT_SPEED = 0.75f;
    constexpr uint32 BLINK_VISUAL = 263;
    constexpr uint32 FIRE_NOVA_VISUAL = 963;
    constexpr float MOLTEN_DEATH_RADIUS = 10.0f;
    constexpr float PULSE_REACH = 12.0f;

    std::string Lower(std::string text)
    {
        for (char& c : text)
            c = char(std::tolower(static_cast<unsigned char>(c)));
        return text;
    }

    uint64_t Uniform(uint64_t n)
    {
        return n <= 1 ? 0 : urand(0, uint32(std::min<uint64_t>(n, 0xFFFFFFFFull) - 1));
    }
}

Affixes::Manager* Affixes::Manager::Instance()
{
    static Manager instance;
    return &instance;
}

bool Affixes::Manager::Load(std::size_t* rows)
{
    auto loaded = std::make_shared<std::map<uint32_t, Template>>();
    QueryResult result = WorldDatabase.Query("SELECT `id`, `name`, `kind`, `aura_spell`, `spell_id`, `period_ms`, `amount_pct`, "
        "`summon_entry`, `summon_count`, `xp_bonus_pct`, `loot_rolls`, `weight`, `hosts` FROM `affix_template`");
    if (result)
    {
        do
        {
            Field* f = result->Fetch();
            Template t;
            t.Id = f[0].GetUInt32();
            t.Name = f[1].GetString();
            uint8 kind = f[2].GetUInt8();
            if (kind < uint8(Kind::Molten) || kind > uint8(Kind::ShaTouched) || !f[3].GetUInt32())
            {
                TC_LOG_ERROR("sql.sql", "affix_template %u: unknown kind %u or no aura_spell, skipped", t.Id, kind);
                continue;
            }
            t.AffixKind = Kind(kind);
            t.AuraSpell = f[3].GetUInt32();
            t.SpellId = f[4].GetUInt32();
            t.PeriodMs = f[5].GetUInt32();
            t.AmountPct = f[6].GetFloat();
            t.SummonEntry = f[7].GetUInt32();
            t.SummonCount = f[8].GetUInt32();
            t.XpBonusPct = f[9].GetUInt32();
            t.LootRolls = f[10].GetUInt32();
            t.Weight = f[11].GetUInt32();
            t.Hosts = f[12].GetUInt32();
            (*loaded)[t.Id] = t;
        } while (result->NextRow());
    }
    if (rows)
        *rows = loaded->size();
    std::lock_guard<std::mutex> guard(_lock);
    // Earlier snapshots stay alive: callers on map threads may still hold a pointer into them. Reloads are rare.
    _retired.push_back(_templates);
    _templates = loaded;
    return true;
}

std::shared_ptr<std::map<uint32_t, Affixes::Template> const> Affixes::Manager::Templates() const
{
    std::lock_guard<std::mutex> guard(_lock);
    return _templates;
}

Affixes::Template const* Affixes::Manager::Find(uint32_t id) const
{
    auto templates = Templates();
    auto itr = templates->find(id);
    return itr == templates->end() ? nullptr : &itr->second;
}

Affixes::Template const* Affixes::Manager::FindByName(std::string const& name) const
{
    std::string wanted = Lower(name);
    if (wanted.empty())
        return nullptr;
    auto templates = Templates();
    for (auto const& entry : *templates)
        if (Lower(entry.second.Name).rfind(wanted, 0) == 0)
            return &entry.second;
    return nullptr;
}

Affixes::Template const* Affixes::Manager::FindByAura(uint32_t auraSpell) const
{
    auto templates = Templates();
    for (auto const& entry : *templates)
        if (entry.second.AuraSpell == auraSpell)
            return &entry.second;
    return nullptr;
}

std::vector<uint32_t> Affixes::Manager::Roll(uint32_t host, uint32_t count) const
{
    std::vector<Candidate> pool;
    auto templates = Templates();
    for (auto const& entry : *templates)
        if ((entry.second.Hosts & host) && entry.second.Weight)
            pool.push_back({ entry.first, entry.second.Weight });
    return RollDistinct(pool, count, Uniform);
}

void Affixes::Manager::Apply(Creature* creature, std::vector<uint32_t> const& affixes, float intensity, uint32_t host)
{
    Clear(creature);
    if (affixes.empty())
        return;
    {
        std::lock_guard<std::mutex> guard(_lock);
        _applied[creature->GetGUID()] = Applied{ affixes, ClampIntensity(intensity), host };
    }
    Reapply(creature);
}

void Affixes::Manager::Reapply(Creature* creature)
{
    for (uint32_t id : Get(creature->GetGUID()))
        if (Template const* t = Find(id))
            if (!creature->HasAura(t->AuraSpell))
                creature->AddAura(t->AuraSpell, creature);
}

void Affixes::Manager::Clear(Creature* creature)
{
    // Auras first: their removal reads the intensity they were applied with (Juggernaut undoes its own health).
    auto templates = Templates();
    for (auto const& entry : *templates)
        creature->RemoveAurasDueToSpell(entry.second.AuraSpell);
    std::lock_guard<std::mutex> guard(_lock);
    _applied.erase(creature->GetGUID());
}

void Affixes::Manager::Forget(ObjectGuid guid)
{
    std::lock_guard<std::mutex> guard(_lock);
    _applied.erase(guid);
}

std::vector<uint32_t> Affixes::Manager::Get(ObjectGuid guid) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _applied.find(guid);
    return itr == _applied.end() ? std::vector<uint32_t>() : itr->second.Ids;
}

float Affixes::Manager::Intensity(ObjectGuid guid) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _applied.find(guid);
    return itr == _applied.end() ? 1.0f : itr->second.Intensity;
}

uint32_t Affixes::Manager::Host(ObjectGuid guid) const
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _applied.find(guid);
    return itr == _applied.end() ? 0 : itr->second.Host;
}

std::string Affixes::Manager::Describe(std::vector<uint32_t> const& affixes) const
{
    std::string text;
    for (uint32_t id : affixes)
        if (Template const* t = Find(id))
            text += (text.empty() ? "" : ", ") + t->Name;
    return text.empty() ? "none" : text;
}

namespace
{
    using Affixes::Kind;
    using Affixes::Template;

    bool HasAmount(Kind kind)
    {
        return kind != Kind::Teleporting && kind != Kind::ShaTouched;
    }

    // The template as this creature runs it: amounts, add counts and amount-less periods follow its intensity.
    Template Scaled(Template t, float intensity)
    {
        t.AmountPct = Affixes::ScaledAmount(t.AmountPct, intensity);
        t.PeriodMs = Affixes::ScaledPeriod(t.PeriodMs, intensity, HasAmount(t.AffixKind));
        t.SummonCount = Affixes::ScaledCount(t.SummonCount, intensity);
        return t;
    }

    void Juggernaut(Creature* me, Template const& t, bool apply)
    {
        float healthMult = 1.0f + std::max(0.0f, t.AmountPct) / 100.0f;
        float healthPct = me->GetHealthPct();
        uint32 maxHealth = apply ? uint32(me->GetMaxHealth() * healthMult) : uint32(me->GetMaxHealth() / healthMult);
        me->SetMaxHealth(std::max<uint32>(1, maxHealth));
        if (me->IsAlive())
            me->SetHealth(std::max<uint32>(1, uint32(me->GetMaxHealth() * healthPct / 100.0f)));
        me->SetObjectScale(apply ? me->GetObjectScale() * JUGGERNAUT_SCALE : me->GetObjectScale() / JUGGERNAUT_SCALE);
        me->SetSpeed(MOVE_RUN, apply ? me->GetSpeedRate(MOVE_RUN) * JUGGERNAUT_SPEED : me->GetSpeedRate(MOVE_RUN) / JUGGERNAUT_SPEED);
        for (uint32 mechanic : { MECHANIC_ROOT, MECHANIC_SNARE, MECHANIC_STUN, MECHANIC_KNOCKOUT })
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, mechanic, apply);
    }

    void Pulse(Creature* me, Template const& t)
    {
        Unit* victim = me->GetVictim();
        if (!victim || !me->IsAlive() || me->HasUnitState(UNIT_STATE_CASTING))
            return;
        switch (t.AffixKind)
        {
            case Kind::Molten:
            case Kind::Frozen:
                if (t.SpellId && me->IsWithinDistInMap(victim, PULSE_REACH))
                    me->CastCustomSpell(t.SpellId, SPELLVALUE_BASE_POINT0, Affixes::ShareOfHealth(victim->GetMaxHealth(), t.AmountPct), me, true);
                break;
            case Kind::Shielded:
                if (t.SpellId)
                    me->CastCustomSpell(t.SpellId, SPELLVALUE_BASE_POINT0, Affixes::ShareOfHealth(me->GetMaxHealth(), t.AmountPct), me, true);
                break;
            case Kind::Teleporting:
            {
                if (me->GetDistance(victim) > 40.0f)
                    break;
                float angle = victim->GetOrientation() + float(M_PI);
                float x = victim->GetPositionX() + 2.5f * std::cos(angle);
                float y = victim->GetPositionY() + 2.5f * std::sin(angle);
                float z = victim->GetPositionZ();
                victim->UpdateAllowedPositionZ(x, y, z);
                me->SendPlaySpellVisual(BLINK_VISUAL, me->GetGUID(), 0.0f);
                me->NearTeleportTo(x, y, z, victim->GetOrientation());
                break;
            }
            case Kind::Summoner:
                if (!t.SummonEntry)
                    break;
                for (uint32 i = 0; i < std::max<uint32>(1, t.SummonCount); ++i)
                {
                    Position pos = me->GetNearPosition(3.0f, float(i) * 2.0f);
                    if (TempSummon* add = me->SummonCreature(t.SummonEntry, pos, TEMPSUMMON_TIMED_OR_CORPSE_DESPAWN, 30000ms))
                    {
                        add->SetLevel(me->GetLevel());
                        add->SetFaction(me->GetFaction());
                        add->SetMaxHealth(std::max<uint32>(1, uint32(me->GetMaxHealth() * std::max(1.0f, t.AmountPct) / 100.0f)));
                        add->SetFullHealth();
                        if (add->AI())
                            add->AI()->AttackStart(victim);
                    }
                }
                break;
            case Kind::ShaTouched:
                if (t.SpellId && me->IsWithinDistInMap(victim, PULSE_REACH))
                    me->CastSpell(me, t.SpellId, true);
                break;
            default:
                break;
        }
    }

    // A dead unit can't cast, so the death blast deals its damage directly and shows the fire nova visual.
    void MoltenDeath(Creature* me, Template const& t)
    {
        std::list<Player*> players;
        me->GetPlayerListInGrid(players, MOLTEN_DEATH_RADIUS);
        me->SendPlaySpellVisual(FIRE_NOVA_VISUAL, me->GetGUID(), 0.0f);
        for (Player* player : players)
            if (player->IsAlive() && !player->IsFriendlyTo(me))
                me->DealDamage(player, uint32(Affixes::ShareOfHealth(player->GetMaxHealth(), t.AmountPct * 2.0f)), nullptr,
                    SPELL_DIRECT_DAMAGE, SPELL_SCHOOL_MASK_FIRE, nullptr, false);
    }
}

// 318001-318099 - affix marker auras (clones of a periodic dummy aura). The aura is the affix: its tick runs the
// mechanic, applying and removing it handles the lasting changes, and its icon shows players what they face.
class spell_affix_marker : public AuraScript
{
    PrepareAuraScript(spell_affix_marker);

    void CalcPeriodic(AuraEffect const* /*aurEff*/, bool& isPeriodic, int32& amplitude)
    {
        Affixes::Template const* t = sAffixMgr->FindByAura(GetId());
        isPeriodic = t && t->PeriodMs;
        if (isPeriodic)
            amplitude = int32(Scaled(*t, sAffixMgr->Intensity(GetUnitOwner()->GetGUID())).PeriodMs);
    }

    void HandlePeriodic(AuraEffect const* /*aurEff*/)
    {
        Creature* me = GetTarget()->ToCreature();
        Affixes::Template const* t = sAffixMgr->FindByAura(GetId());
        if (me && t && me->IsInCombat())
            Pulse(me, Scaled(*t, sAffixMgr->Intensity(me->GetGUID())));
    }

    void HandleApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Creature* me = GetTarget()->ToCreature();
        Affixes::Template const* t = sAffixMgr->FindByAura(GetId());
        if (me && t && t->AffixKind == Kind::Juggernaut)
            Juggernaut(me, Scaled(*t, sAffixMgr->Intensity(me->GetGUID())), true);
    }

    void HandleRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
    {
        Creature* me = GetTarget()->ToCreature();
        Affixes::Template const* t = sAffixMgr->FindByAura(GetId());
        if (!me || !t)
            return;
        Template scaled = Scaled(*t, sAffixMgr->Intensity(me->GetGUID()));
        if (t->AffixKind == Kind::Juggernaut)
            Juggernaut(me, scaled, false);
        if (t->AffixKind == Kind::Molten && GetTargetApplication()->GetRemoveMode() == AURA_REMOVE_BY_DEATH)
            MoltenDeath(me, scaled);
    }

    void Register() override
    {
        DoEffectCalcPeriodic += AuraEffectCalcPeriodicFn(spell_affix_marker::CalcPeriodic, EFFECT_0, SPELL_AURA_ANY);
        OnEffectPeriodic += AuraEffectPeriodicFn(spell_affix_marker::HandlePeriodic, EFFECT_0, SPELL_AURA_ANY);
        AfterEffectApply += AuraEffectApplyFn(spell_affix_marker::HandleApply, EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
        AfterEffectRemove += AuraEffectRemoveFn(spell_affix_marker::HandleRemove, EFFECT_0, SPELL_AURA_ANY, AURA_EFFECT_HANDLE_REAL);
    }
};

// Vampiric: the affixed creature heals for a share of the damage it deals.
class affix_unitscript : public UnitScript
{
public:
    affix_unitscript() : UnitScript("affix_unitscript") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!damage || !attacker || attacker == victim || attacker->GetTypeId() != TYPEID_UNIT || !attacker->IsAlive())
            return;
        for (uint32 id : sAffixMgr->Get(attacker->GetGUID()))
            if (Affixes::Template const* t = sAffixMgr->Find(id))
                if (t->AffixKind == Kind::Vampiric)
                    attacker->ModifyHealth(int32(damage * Affixes::ScaledAmount(t->AmountPct, sAffixMgr->Intensity(attacker->GetGUID())) / 100.0f));
    }
};

void AddSC_affixes()
{
    new affix_unitscript();
    new aura_script<spell_affix_marker>("spell_affix_marker");
}
