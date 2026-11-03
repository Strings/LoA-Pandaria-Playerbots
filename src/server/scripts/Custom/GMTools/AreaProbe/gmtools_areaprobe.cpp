/*
 * Area probe: `.areaprobe place` stands rooted probe dummies on rings and spokes around the GM, every spell that
 * touches one is recorded with where its caster stood, and `.areaprobe report` turns the hits and misses into the
 * spell's reach, cone angle and circle. A spell's area can be measured and tuned without anyone eyeballing it.
 * See README.md.
 */

#include "AreaProbeRules.h"
#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "CreatureAI.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "TemporarySummon.h"
#include "WorldSession.h"

#include <map>
#include <mutex>
#include <set>
#include <sstream>

// GM tools block (docs/ID_RANGES.md); linked to role 197 by sql/updates/master/auth/2026_11_03_claude_00_areaprobe_command.sql
static rbac::RBACPermissions const RBAC_PERM_COMMAND_AREAPROBE = static_cast<rbac::RBACPermissions>(91010);

namespace
{
    uint32 const ProbeEntry = 31144;    // a training dummy: a stationary target model
    uint32 const FactionHostile = 14;
    uint32 const FactionFriendly = 35;

    struct SpellRecord
    {
        std::set<ObjectGuid> Probes;
        float CasterX = 0.0f;           // where its caster stood at the latest hit
        float CasterY = 0.0f;
        float CasterO = 0.0f;
        std::string CasterName;
        uint32 Order = 0;
    };

    struct ProbeField
    {
        std::map<ObjectGuid, uint32> Rays;  // probe -> spoke
        std::map<uint32, SpellRecord> Spells;
        uint32 Seen = 0;
    };

    // Probes record from map threads; commands read from the world thread.
    std::mutex Lock;
    std::map<ObjectGuid, ProbeField> Fields;    // GM -> the probes they placed
    std::map<ObjectGuid, ObjectGuid> Owners;    // probe -> GM

    void Record(Creature* probe, Unit* caster, SpellInfo const* spell)
    {
        if (!caster || !spell)
            return;
        std::lock_guard<std::mutex> guard(Lock);
        auto owner = Owners.find(probe->GetGUID());
        if (owner == Owners.end())
            return;
        ProbeField& field = Fields[owner->second];
        SpellRecord& record = field.Spells[spell->Id];
        if (record.Probes.empty())
            record.Order = ++field.Seen;
        record.Probes.insert(probe->GetGUID());
        record.CasterX = caster->GetPositionX();
        record.CasterY = caster->GetPositionY();
        record.CasterO = caster->GetOrientation();
        record.CasterName = caster->GetName();
    }

    // Stands still and takes no damage: knockbacks and pulls would move it off its spoke, so it ignores them.
    class AreaProbeAI : public CreatureAI
    {
    public:
        explicit AreaProbeAI(Creature* creature) : CreatureAI(creature) { }

        void Reset() override
        {
            me->SetReactState(REACT_PASSIVE);
            me->SetControlled(true, UNIT_STATE_ROOT);
            for (uint32 effect : { SPELL_EFFECT_KNOCK_BACK, SPELL_EFFECT_KNOCK_BACK_DEST, SPELL_EFFECT_PULL_TOWARDS,
                SPELL_EFFECT_PULL_TOWARDS_DEST })
                me->ApplySpellImmune(0, IMMUNITY_EFFECT, effect, true);
        }

        void UpdateAI(uint32 /*diff*/) override { }
        void MoveInLineOfSight(Unit* /*who*/) override { }
        void AttackStart(Unit* /*target*/) override { }
        void EnterEvadeMode(EvadeReason /*why*/) override { }
        void DamageTaken(Unit* /*attacker*/, uint32& damage) override { damage = 0; }
        void SpellHit(Unit* caster, SpellInfo const* spell) override { Record(me, caster, spell); }
    };

    std::vector<std::string> Words(char const* args)
    {
        std::vector<std::string> words;
        std::istringstream in(args ? args : "");
        for (std::string word; in >> word;)
            words.push_back(word);
        return words;
    }

    std::string RangeText(AreaProbe::Range range, float scale, char const* unit)
    {
        char buffer[64];
        if (range.High == AreaProbe::Unbounded)
            snprintf(buffer, sizeof(buffer), "%.1f+ %s (no probe beyond missed)", range.Low * scale, unit);
        else
            snprintf(buffer, sizeof(buffer), "%.1f-%.1f %s", range.Low * scale, range.High * scale, unit);
        return buffer;
    }

    float const Degrees = 180.0f / AreaProbe::Pi;
}

class gmtools_areaprobe_commandscript : public CommandScript
{
public:
    gmtools_areaprobe_commandscript() : CommandScript("gmtools_areaprobe_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> areaProbeCommandTable =
        {
            { "place",  &HandlePlaceCommand,  RBAC_PERM_COMMAND_AREAPROBE, Trinity::ChatCommands::Console::No },
            { "report", &HandleReportCommand, RBAC_PERM_COMMAND_AREAPROBE, Trinity::ChatCommands::Console::No },
            { "reset",  &HandleResetCommand,  RBAC_PERM_COMMAND_AREAPROBE, Trinity::ChatCommands::Console::No },
            { "clear",  &HandleClearCommand,  RBAC_PERM_COMMAND_AREAPROBE, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "areaprobe", areaProbeCommandTable, RBAC_PERM_COMMAND_AREAPROBE, Trinity::ChatCommands::Console::No },
        };
        return commandTable;
    }

    static uint32 Clear(Player* player)
    {
        std::map<ObjectGuid, uint32> rays;
        {
            std::lock_guard<std::mutex> guard(Lock);
            auto itr = Fields.find(player->GetGUID());
            if (itr == Fields.end())
                return 0;
            rays = std::move(itr->second.Rays);
            Fields.erase(itr);
            for (auto const& [probe, ray] : rays)
                Owners.erase(probe);
        }
        // Probes left behind on another map despawn when their lifetime runs out.
        uint32 removed = 0;
        for (auto const& [probe, ray] : rays)
            if (Creature* creature = ObjectAccessor::GetCreature(*player, probe))
            {
                creature->DespawnOrUnsummon();
                ++removed;
            }
        return removed;
    }

    // .areaprobe place [step yd = 2] [rings = 8] [spokes = 16] [arc deg = 360] [friendly]
    static bool HandlePlaceCommand(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> words = Words(args);
        bool friendly = !words.empty() && words.back() == "friendly";
        if (friendly)
            words.pop_back();
        float step = words.size() > 0 ? float(atof(words[0].c_str())) : 2.0f;
        uint32 rings = words.size() > 1 ? uint32(atoi(words[1].c_str())) : 8;
        uint32 rays = words.size() > 2 ? uint32(atoi(words[2].c_str())) : 16;
        float arc = (words.size() > 3 ? float(atof(words[3].c_str())) : 360.0f) / Degrees;

        uint32 maxProbes = sConfigMgr->GetIntDefault("GMTools.AreaProbe.MaxProbes", 400);
        if (step < 0.5f || !rings || !rays || arc <= 0.0f || rings * rays > maxProbes)
        {
            handler->PSendSysMessage("Usage: .areaprobe place [step yd >= 0.5] [rings] [spokes] [arc deg] [friendly]; at most %u probes (GMTools.AreaProbe.MaxProbes).", maxProbes);
            handler->SetSentErrorMessage(true);
            return false;
        }

        Player* player = handler->GetSession()->GetPlayer();
        Clear(player);
        Milliseconds lifetime(uint64(std::max(1, sConfigMgr->GetIntDefault("GMTools.AreaProbe.Lifetime", 600))) * IN_MILLISECONDS);
        float o = player->GetOrientation();
        uint32 placed = 0;
        float size = 0.0f;
        for (AreaProbe::Slot const& slot : AreaProbe::Layout(step, rings, rays, arc))
        {
            float x = player->GetPositionX() + slot.Distance * std::cos(o + slot.Angle);
            float y = player->GetPositionY() + slot.Distance * std::sin(o + slot.Angle);
            float z = player->GetPositionZ();
            player->UpdateAllowedPositionZ(x, y, z);
            TempSummon* probe = player->SummonCreature(ProbeEntry, x, y, z, Position::NormalizeOrientation(o + slot.Angle + float(M_PI)),
                TEMPSUMMON_TIMED_DESPAWN, lifetime);
            if (!probe)
                continue;
            probe->SetFaction(friendly ? FactionFriendly : FactionHostile);
            probe->AIM_Initialize(new AreaProbeAI(probe));
            size = probe->GetObjectSize();
            std::lock_guard<std::mutex> guard(Lock);
            Fields[player->GetGUID()].Rays[probe->GetGUID()] = slot.Ray;
            Owners[probe->GetGUID()] = player->GetGUID();
            ++placed;
        }
        handler->PSendSysMessage("Area probe: placed %u %s probes, rings every %.1f yd to %.1f yd on %u spokes over %.0f deg; probe size %.2f yd. Cast, then .areaprobe report.",
            placed, friendly ? "friendly" : "hostile", step, step * rings, rays, arc * Degrees, size);
        return true;
    }

    static void ReportSpell(ChatHandler* handler, Player* player, ProbeField const& field, uint32 spellId, SpellRecord const& record)
    {
        std::vector<AreaProbe::Sample> samples;
        for (auto const& [probe, ray] : field.Rays)
            if (Creature* creature = ObjectAccessor::GetCreature(*player, probe))
                samples.push_back({ AreaProbe::ToLocal(record.CasterX, record.CasterY, record.CasterO, creature->GetPositionX(), creature->GetPositionY()),
                    ray, record.Probes.count(probe) > 0 });
        AreaProbe::Fit fit = AreaProbe::Analyze(samples);

        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
        handler->PSendSysMessage("Area probe: spell %u (%s) hit %u of %u probes on %u of %u spokes; caster %s.", spellId,
            info ? info->SpellName : "?", fit.Hits, fit.Hits + fit.Misses, fit.RaysHit, fit.Rays, record.CasterName.c_str());
        if (!fit.Hits)
            return;
        if (fit.FullCircle)
            handler->PSendSysMessage("  Circle around the caster: reach %s.", RangeText(fit.Reach, 1.0f, "yd").c_str());
        else
            handler->PSendSysMessage("  Cone: centered %.0f deg from facing, half-angle %s, reach %s.", fit.Center * Degrees,
                RangeText(fit.HalfAngle, Degrees, "deg").c_str(), RangeText(fit.Reach, 1.0f, "yd").c_str());
        handler->PSendSysMessage("  Free circle: center %.1f yd ahead, %.1f yd left; radius %s.", fit.CircleCenter.X, fit.CircleCenter.Y,
            RangeText(fit.Radius, 1.0f, "yd").c_str());
        if (fit.Gaps)
            handler->SendSysMessage("  Gaps: a spoke missed inside its farthest hit (minimum range, line of sight, a target cap, or an area away from the caster).");
    }

    // .areaprobe report [spell id]: every spell that touched the probes, oldest first, or just the one asked for.
    static bool HandleReportCommand(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> words = Words(args);
        uint32 only = words.empty() ? 0 : uint32(atoi(words[0].c_str()));
        Player* player = handler->GetSession()->GetPlayer();

        ProbeField field;
        {
            std::lock_guard<std::mutex> guard(Lock);
            auto itr = Fields.find(player->GetGUID());
            if (itr == Fields.end())
            {
                handler->SendSysMessage("Area probe: no probes placed; .areaprobe place first.");
                handler->SetSentErrorMessage(true);
                return false;
            }
            field = itr->second;
        }

        std::map<uint32, uint32> byOrder;
        for (auto const& [spellId, record] : field.Spells)
            if (!only || spellId == only)
                byOrder[record.Order] = spellId;
        if (byOrder.empty())
        {
            handler->PSendSysMessage("Area probe: no spell%s has touched the %u probes yet.", only ? " with that id" : "", uint32(field.Rays.size()));
            return true;
        }
        handler->SendSysMessage("Distances are to probe centers; most area checks also count the target's size.");
        for (auto const& [order, spellId] : byOrder)
            ReportSpell(handler, player, field, spellId, field.Spells.at(spellId));
        return true;
    }

    static bool HandleResetCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        std::lock_guard<std::mutex> guard(Lock);
        auto itr = Fields.find(player->GetGUID());
        if (itr != Fields.end())
        {
            itr->second.Spells.clear();
            itr->second.Seen = 0;
        }
        handler->SendSysMessage("Area probe: hits forgotten; probes stay.");
        return true;
    }

    static bool HandleClearCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->PSendSysMessage("Area probe: removed %u probes.", Clear(handler->GetSession()->GetPlayer()));
        return true;
    }
};

void AddSC_gmtools_areaprobe()
{
    new gmtools_areaprobe_commandscript();
}
