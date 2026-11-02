/*
 * Heartseed rules that need nothing from the server: stage by level, dominant path, trait activation and
 * the amounts traits and the core passive give.
 * Kept free of game headers so the unit test framework (T-008) can test them directly.
 */

#ifndef HEARTSEED_RULES_H
#define HEARTSEED_RULES_H

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <vector>

namespace Heartseed
{
    enum class Path : uint8_t
    {
        None    = 0,
        Xuen    = 1,   // offense and speed
        ChiJi   = 2,   // healing and hope
        Niuzao  = 3,   // endurance and defense
        Yulon   = 4,   // wisdom and spell power
    };

    constexpr uint8_t MaxPaths = 5;          // including None
    constexpr uint8_t MaxMajorTiers = 6;     // majors at 10, 25, 40, 55, 70, 85 by default
    constexpr uint8_t MaxEssenceSlots = 3;   // slots open at 10, 40, 70 by default

    // Character levels that open each major tier and each essence slot. The counts are fixed (saved data uses
    // the tier and slot indexes); the levels come from Heartseed.Majors.TierLevels and
    // Heartseed.Essence.SlotLevels, set once at config load.
    struct UnlockLevels
    {
        std::array<uint8_t, MaxMajorTiers> MajorTiers = { 10, 25, 40, 55, 70, 85 };
        std::array<uint8_t, MaxEssenceSlots> EssenceSlots = { 10, 40, 70 };
    };

    inline UnlockLevels& Unlocks()
    {
        static UnlockLevels levels;
        return levels;
    }

    // Accepts exactly count levels from 1 to 255, ascending; anything else keeps the current levels.
    template <size_t N>
    inline bool SetLevels(std::array<uint8_t, N>& levels, std::vector<uint32_t> const& values)
    {
        if (values.size() != N)
            return false;
        for (size_t i = 0; i < N; ++i)
            if (!values[i] || values[i] > 255 || (i && values[i] < values[i - 1]))
                return false;
        for (size_t i = 0; i < N; ++i)
            levels[i] = uint8_t(values[i]);
        return true;
    }

    // Stage is the index of the last stage whose minimum level the character has reached.
    // stageMinLevels must be sorted ascending; levels below the first entry still count as stage 0.
    inline uint8_t StageForLevel(std::vector<uint8_t> const& stageMinLevels, uint8_t level)
    {
        uint8_t stage = 0;
        for (size_t i = 0; i < stageMinLevels.size(); ++i)
            if (level >= stageMinLevels[i])
                stage = uint8_t(i);
        return stage;
    }

    // The dominant path is the one with the most major choices; a tie means no single path dominates.
    inline Path DominantPath(std::array<uint8_t, MaxPaths> const& majorsPerPath)
    {
        Path best = Path::None;
        uint8_t bestCount = 0;
        bool tie = false;
        for (uint8_t p = uint8_t(Path::Xuen); p < MaxPaths; ++p)
        {
            if (majorsPerPath[p] > bestCount)
            {
                best = Path(p);
                bestCount = majorsPerPath[p];
                tie = false;
            }
            else if (majorsPerPath[p] && majorsPerPath[p] == bestCount)
                tie = true;
        }
        return tie ? Path::None : best;
    }

    enum class TraitKind : uint8_t
    {
        Minor = 1,   // fills in by rank along the paths the character walks
        Major = 2,   // chosen, one per major tier
    };

    // Character level at which a major tier opens; by default between MoP's talent tiers (15, 30, ...).
    inline uint8_t MajorTierLevel(uint8_t tier)
    {
        return tier < MaxMajorTiers ? Unlocks().MajorTiers[tier] : 0;
    }

    // Tier index for a tier's opening level, or MaxMajorTiers if the level opens no tier. With two tiers at the
    // same level, the first.
    inline uint8_t MajorTierForLevel(uint8_t level)
    {
        for (uint8_t tier = 0; tier < MaxMajorTiers; ++tier)
            if (Unlocks().MajorTiers[tier] == level)
                return tier;
        return MaxMajorTiers;
    }

    inline bool MajorTierOpen(uint8_t tier, uint8_t level)
    {
        return tier < MaxMajorTiers && level >= Unlocks().MajorTiers[tier];
    }

    // A minor fills in at a character level, on a path the character has taken at least one major in, so an alt
    // or a twink has the balance it would have alone (no account rank, T-083). Path None: every Heartseed gets it.
    inline bool MinorActive(Path path, uint8_t minLevel, uint8_t level, std::array<uint8_t, MaxPaths> const& majorsPerPath)
    {
        if (level < minLevel)
            return false;
        return path == Path::None || majorsPerPath[uint8_t(path)] > 0;
    }

    // Trait strength grows with the main-hand weapon's item level, so a better weapon strengthens the jewel.
    inline int32_t TraitAmount(int32_t baseAmount, float amountPerItemLevel, uint32_t itemLevel)
    {
        return baseAmount + int32_t(std::lround(amountPerItemLevel * float(itemLevel)));
    }

    // Core passive: extra damage per swing as a share of the weapon's own average damage, which already
    // follows item level.
    inline float CoreWeaponBonus(float weaponMinDamage, float weaponMaxDamage, float bonusPct)
    {
        if (weaponMaxDamage <= 0.0f || bonusPct <= 0.0f)
            return 0.0f;
        return (weaponMinDamage + weaponMaxDamage) * 0.5f * bonusPct / 100.0f;
    }

    // Core passive proc: a share of the weapon damage the character currently deals per swing.
    inline uint32_t CoreProcDamage(float weaponMinDamage, float weaponMaxDamage, float damagePct)
    {
        float damage = (weaponMinDamage + weaponMaxDamage) * 0.5f * damagePct / 100.0f;
        return damage > 0.0f ? uint32_t(std::lround(damage)) : 0;
    }

    inline uint8_t EssenceSlotsForLevel(uint8_t level)
    {
        uint8_t slots = 0;
        for (uint8_t openLevel : Unlocks().EssenceSlots)
            if (level >= openLevel)
                ++slots;
        return slots;
    }

    // Growth XP (DESIGN.md Progression, T-083): battleground wins and dungeon and raid bosses near the character's
    // level fill a bar; each Growth level is one stat point the player spends. No cap: the cost per level rises.

    // Growth XP needed to go from level to level + 1. Never 0, so a level-up loop always ends.
    inline uint32_t GrowthXpForLevel(uint32_t level, uint32_t base, uint32_t perLevel)
    {
        uint64_t cost = uint64_t(base) + uint64_t(perLevel) * uint64_t(level);
        if (cost == 0)
            return 1;
        return cost > UINT32_MAX ? UINT32_MAX : uint32_t(cost);
    }

    struct GrowthProgress
    {
        uint32_t Level;
        uint32_t Xp;          // left over toward the next level
    };

    inline GrowthProgress ApplyGrowthXp(uint32_t level, uint32_t xp, uint32_t base, uint32_t perLevel)
    {
        while (level < UINT32_MAX)
        {
            uint32_t cost = GrowthXpForLevel(level, base, perLevel);
            if (xp < cost)
                break;
            xp -= cost;
            ++level;
        }
        return { level, xp };
    }

    // A source counts when it is at most `range` levels below the character; a higher one always counts.
    inline bool GrowthSourceNearLevel(uint8_t characterLevel, uint8_t sourceLevel, uint8_t range)
    {
        return uint32_t(sourceLevel) + range >= characterLevel;
    }

    // One point per Growth level; points already spent (stat id -> points) come off.
    inline uint32_t UnspentPoints(uint32_t level, std::map<uint8_t, uint32_t> const& spent)
    {
        uint64_t total = 0;
        for (auto const& entry : spent)
            total += entry.second;
        return total >= level ? 0 : uint32_t(level - total);
    }

    // The stat aura's amount: points times percent per point, saturating at INT32_MAX.
    inline int32_t StatPointAmount(uint32_t points, int32_t pctPerPoint)
    {
        if (pctPerPoint <= 0)
            return 0;
        int64_t amount = int64_t(points) * pctPerPoint;
        return amount > INT32_MAX ? INT32_MAX : int32_t(amount);
    }

    // The main stat a "main stat" point raises: the specialization's, or the class's usual one before a spec is
    // chosen. Values are the core's Stats (0 strength, 1 agility, 3 intellect).
    enum class MainStat : uint8_t { Strength = 0, Agility = 1, Intellect = 3 };

    inline MainStat MainStatFor(uint8_t classId, uint32_t specId)
    {
        switch (specId)
        {
            case 66: case 70: case 71: case 72: case 73: case 250: case 251: case 252:
                return MainStat::Strength;
            case 103: case 104: case 253: case 254: case 255: case 259: case 260: case 261: case 263: case 268: case 269:
                return MainStat::Agility;
            case 62: case 63: case 64: case 65: case 102: case 105: case 256: case 257: case 258: case 262: case 264:
            case 265: case 266: case 267: case 270:
                return MainStat::Intellect;
            default:
                break;
        }
        switch (classId)
        {
            case 1: case 2: case 6:                   // warrior, paladin, death knight
                return MainStat::Strength;
            case 3: case 4: case 10:                  // hunter, rogue, monk
                return MainStat::Agility;
            default:
                return MainStat::Intellect;
        }
    }

    // Saturating add, for Growth XP and essence counts.
    inline uint32_t AddCapped(uint32_t value, uint32_t amount)
    {
        return amount > UINT32_MAX - value ? UINT32_MAX : value + amount;
    }

    // First essence slot (1-based) that is open at this level and not taken, or 0 when none is free.
    inline uint8_t FreeEssenceSlot(std::vector<uint8_t> const& takenSlots, uint8_t level)
    {
        uint8_t open = EssenceSlotsForLevel(level);
        for (uint8_t slot = 1; slot <= open; ++slot)
        {
            bool taken = false;
            for (uint8_t used : takenSlots)
                taken = taken || used == slot;
            if (!taken)
                return slot;
        }
        return 0;
    }

    // Celestial Gift riders (heartseed_gift_rider): each Heartseed stage adds one to the Gift (DESIGN.md, The Gift
    // grows with the seed). The kind says which hook applies it; amount, duration and helper spell are data.
    enum class GiftRiderKind : uint8_t
    {
        SelfAura        = 1,   // on cast: helper aura on the caster, effect 0 = amount, for duration
        ExtraTargets    = 2,   // the Gift's damage spell hits amount more enemies near its targets
        KillCooldown    = 3,   // a kill cuts the Gift cooldown by amount ms; 0 resets it
        HealPct         = 4,   // on cast: heal amount % of max health
        TargetAura      = 5,   // each enemy hit gets the helper aura, effect 0 = amount, for duration
        AbsorbPerTarget = 6,   // after the cast: helper absorb of amount % of max health per enemy hit
        Widen           = 7,   // area grows by amount %, and its aura lasts duration ms longer
        LeechPct        = 8,   // heal amount % of the Gift's damage
        TargetDot       = 9,   // each enemy hit burns: helper DoT ticking amount % of the Gift's power
        HitCooldown     = 10,  // each distinct enemy hit cuts the Gift cooldown by amount ms
        RecentDamageShield = 11, // on cast: helper absorb of amount % of the damage taken in the meter window
    };

    constexpr uint8_t MaxGiftRiderKind = 11;

    struct GiftRider
    {
        uint8_t Stage = 0;                    // heartseed_stage index that adds it
        GiftRiderKind Kind = GiftRiderKind::SelfAura;
        int32_t Amount = 0;
        uint32_t Duration = 0;                // ms; 0 keeps the helper spell's own
        uint32_t SpellId = 0;                 // helper spell, for the kinds that apply one
    };

    // Riders reached at this stage; a character keeps every earlier stage's rider.
    inline std::vector<GiftRider> ActiveRiders(std::vector<GiftRider> const& riders, uint8_t stage)
    {
        std::vector<GiftRider> active;
        for (GiftRider const& rider : riders)
            if (rider.Stage <= stage)
                active.push_back(rider);
        return active;
    }

    inline uint32_t PercentOf(uint32_t value, int32_t pct)
    {
        if (pct <= 0)
            return 0;
        uint64_t result = uint64_t(value) * uint64_t(pct) / 100;
        return result > UINT32_MAX ? UINT32_MAX : uint32_t(result);
    }

    // How far to cut a cooldown with remainingMs left: never past ready, and a cut of 0 means a full reset.
    inline uint32_t CooldownCut(uint32_t remainingMs, int32_t cutMs)
    {
        if (cutMs <= 0)
            return remainingMs;
        return uint32_t(cutMs) < remainingMs ? uint32_t(cutMs) : remainingMs;
    }

    // Path meters (heartseed_meter, DESIGN.md "Active play, not big cooldowns"): normal play of any class builds
    // the meter of the Gift's path, and casting the Gift spends it. Times are getMSTime() values; differences
    // are taken unsigned, so the wrap after 49 days is harmless.
    enum class MeterSource : uint8_t
    {
        None      = 0,
        Crit      = 1,   // each critical hit dealt adds a stack
        Healing   = 2,   // healing done or received adds a stack per threshold % of max health
        BigHit    = 3,   // a hit taken of at least threshold % of max health lights the Gift
        SpellCast = 4,   // each spell cast (not the Gift, not triggered) adds a stack
    };

    constexpr uint8_t MaxMeterSource = 4;

    struct MeterRule
    {
        MeterSource Source = MeterSource::None;
        uint8_t MaxStacks = 0;                // 0 = the meter has no stacks (BigHit lights instead)
        uint32_t ThresholdPct = 0;            // Healing: % of max health per stack; BigHit: % of max health
        uint32_t LitMs = 0;                   // BigHit: how long the Gift stays usable; 0 = never gated
        uint32_t DecayMs = 0;                 // stacks drop this long after the last build; 0 = never
        uint32_t BuildCooldownMs = 0;         // minimum time between two builds
        int32_t SpendBonusPct = 0;            // Gift power per stack spent
        int32_t CritCooldownCutMs = 0;        // each crit cuts the Gift cooldown by this much (shares BuildCooldownMs)
        uint32_t WindowMs = 0;                // damage taken is remembered this long (Retort)
    };

    struct MeterState
    {
        uint8_t Stacks = 0;
        uint32_t LastBuild = 0;
        bool HasBuilt = false;
        uint32_t LitAt = 0;
        bool Lit = false;
        uint32_t HealAccum = 0;               // healing toward the next stack
        uint32_t LastCritCut = 0;
        bool HasCritCut = false;
    };

    inline bool Elapsed(uint32_t since, uint32_t now, uint32_t ms)
    {
        return uint32_t(now - since) >= ms;
    }

    // Drops stacks and healing that sat past the decay time, and a lit window that ran out.
    inline void MeterDecay(MeterState& meter, MeterRule const& rule, uint32_t now)
    {
        if (rule.DecayMs && meter.HasBuilt && Elapsed(meter.LastBuild, now, rule.DecayMs))
        {
            meter.Stacks = 0;
            meter.HealAccum = 0;
        }
        if (meter.Lit && Elapsed(meter.LitAt, now, rule.LitMs))
            meter.Lit = false;
    }

    // One build event. Returns true when the meter changed.
    inline bool MeterBuild(MeterState& meter, MeterRule const& rule, uint32_t now, uint8_t stacks = 1)
    {
        MeterDecay(meter, rule, now);
        if (meter.HasBuilt && rule.BuildCooldownMs && !Elapsed(meter.LastBuild, now, rule.BuildCooldownMs))
            return false;
        bool changed = false;
        if (rule.LitMs)
        {
            changed = !meter.Lit || meter.LitAt != now;
            meter.Lit = true;
            meter.LitAt = now;
        }
        if (rule.MaxStacks && stacks)
        {
            uint8_t before = meter.Stacks;
            meter.Stacks = uint8_t(std::min<uint32_t>(uint32_t(meter.Stacks) + stacks, rule.MaxStacks));
            changed = changed || meter.Stacks != before;
        }
        meter.LastBuild = now;
        meter.HasBuilt = true;
        return changed;
    }

    // Healing toward Hope: every threshold % of max health healed is one stack; the rest carries over.
    inline uint8_t HealingStacks(MeterState& meter, MeterRule const& rule, uint32_t amount, uint32_t maxHealth)
    {
        uint32_t step = PercentOf(maxHealth, int32_t(rule.ThresholdPct));
        if (!step)
            return 0;
        uint64_t total = uint64_t(meter.HealAccum) + amount;
        uint64_t stacks = total / step;
        meter.HealAccum = uint32_t(total % step);
        return uint8_t(std::min<uint64_t>(stacks, 255));
    }

    inline bool IsBigHit(MeterRule const& rule, uint32_t damage, uint32_t maxHealth)
    {
        uint32_t needed = PercentOf(maxHealth, int32_t(rule.ThresholdPct));
        return needed && damage >= needed;
    }

    // The Gift may be cast: a gated meter (LitMs) must be lit.
    inline bool MeterAllowsCast(MeterState meter, MeterRule const& rule, uint32_t now)
    {
        if (!rule.LitMs)
            return true;
        MeterDecay(meter, rule, now);
        return meter.Lit;
    }

    // Casting the Gift spends the meter. Returns the stacks spent.
    inline uint8_t MeterSpend(MeterState& meter, MeterRule const& rule, uint32_t now)
    {
        MeterDecay(meter, rule, now);
        uint8_t spent = meter.Stacks;
        meter.Stacks = 0;
        meter.HealAccum = 0;
        meter.Lit = false;
        return spent;
    }

    // Gift power with the spent stacks' bonus.
    inline uint32_t SpendScaled(uint32_t value, uint8_t stacks, int32_t bonusPctPerStack)
    {
        int64_t pct = 100 + int64_t(stacks) * bonusPctPerStack;
        if (pct <= 0)
            return 0;
        uint64_t result = uint64_t(value) * uint64_t(pct) / 100;
        return result > UINT32_MAX ? UINT32_MAX : uint32_t(result);
    }

    // A crit's Gift cooldown cut, at most once per BuildCooldownMs. Returns the cut in ms, 0 for none.
    inline int32_t CritCut(MeterState& meter, MeterRule const& rule, uint32_t now)
    {
        if (rule.CritCooldownCutMs <= 0)
            return 0;
        if (meter.HasCritCut && rule.BuildCooldownMs && !Elapsed(meter.LastCritCut, now, rule.BuildCooldownMs))
            return 0;
        meter.LastCritCut = now;
        meter.HasCritCut = true;
        return rule.CritCooldownCutMs;
    }

    // Damage taken in the last windowMs; entries are (time, amount), oldest first, and old ones are dropped.
    inline uint32_t RecentDamage(std::vector<std::pair<uint32_t, uint32_t>>& hits, uint32_t now, uint32_t windowMs)
    {
        size_t keep = 0;
        while (keep < hits.size() && Elapsed(hits[keep].first, now, windowMs + 1))
            ++keep;
        hits.erase(hits.begin(), hits.begin() + keep);
        uint64_t total = 0;
        for (auto const& hit : hits)
            total += hit.second;
        return total > UINT32_MAX ? UINT32_MAX : uint32_t(total);
    }
}

#endif
