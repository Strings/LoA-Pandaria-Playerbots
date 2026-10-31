/*
 * Heartseed rules that need nothing from the server: stage by level, dominant path, trait activation and
 * the amounts traits and the core passive give.
 * Kept free of game headers so the unit test framework (T-008) can test them directly.
 */

#ifndef HEARTSEED_RULES_H
#define HEARTSEED_RULES_H

#include <array>
#include <cmath>
#include <cstdint>
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
    constexpr uint8_t MaxMajorTiers = 6;     // majors at 10, 25, 40, 55, 70, 85
    constexpr uint8_t MaxEssenceSlots = 3;   // slots open at 10, 40, 70

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

    // Character level at which each major tier opens, between MoP's talent tiers (15, 30, ...).
    constexpr std::array<uint8_t, MaxMajorTiers> MajorTierLevels = { 10, 25, 40, 55, 70, 85 };

    // Tier index for a tier's opening level, or MaxMajorTiers if the level opens no tier.
    inline uint8_t MajorTierForLevel(uint8_t level)
    {
        for (uint8_t tier = 0; tier < MaxMajorTiers; ++tier)
            if (MajorTierLevels[tier] == level)
                return tier;
        return MaxMajorTiers;
    }

    inline bool MajorTierOpen(uint8_t tier, uint8_t level)
    {
        return tier < MaxMajorTiers && level >= MajorTierLevels[tier];
    }

    // A minor fills in once the rank reaches it, on a path the character has taken at least one major in.
    // Path None marks a minor every Heartseed gets.
    inline bool MinorActive(Path path, uint8_t minRank, uint8_t rank, std::array<uint8_t, MaxPaths> const& majorsPerPath)
    {
        if (rank < minRank)
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

    constexpr uint8_t MaxRank = 255;           // rank is saved as TINYINT UNSIGNED

    // Character level at which each essence slot opens (1 at 10, 2 at 40, 3 at 70).
    constexpr std::array<uint8_t, MaxEssenceSlots> EssenceSlotLevels = { 10, 40, 70 };

    inline uint8_t EssenceSlotsForLevel(uint8_t level)
    {
        uint8_t slots = 0;
        for (uint8_t openLevel : EssenceSlotLevels)
            if (level >= openLevel)
                ++slots;
        return slots;
    }

    // Rank cap: the character level sets it, each awakening (story flag that counts) raises it.
    // Never below 1, so a fresh Heartseed always has its first rank.
    inline uint8_t RankCap(uint8_t level, float capPerLevel, uint32_t awakenings, uint32_t capPerAwakening)
    {
        double cap = std::floor(double(level) * double(capPerLevel)) + double(awakenings) * double(capPerAwakening);
        if (cap < 1.0)
            return 1;
        return cap > double(MaxRank) ? MaxRank : uint8_t(cap);
    }

    // Growth needed to go from rank to rank + 1. Never 0, so a rank-up loop always ends.
    inline uint32_t GrowthForNextRank(uint8_t rank, uint32_t base, uint32_t perRank)
    {
        uint64_t cost = uint64_t(base) + uint64_t(perRank) * uint64_t(rank > 0 ? rank - 1 : 0);
        if (cost == 0)
            return 1;
        return cost > UINT32_MAX ? UINT32_MAX : uint32_t(cost);
    }

    struct RankProgress
    {
        uint8_t Rank;
        uint32_t Growth;      // left over toward the next rank
    };

    // Spends Growth on ranks up to the cap. Growth past the cap stays banked, so raising the cap (a level
    // or an awakening) ranks the Heartseed up at once. A rank already above the cap (GM) is kept.
    inline RankProgress ApplyGrowth(uint8_t rank, uint32_t growth, uint8_t cap, uint32_t base, uint32_t perRank)
    {
        while (rank < cap)
        {
            uint32_t cost = GrowthForNextRank(rank, base, perRank);
            if (growth < cost)
                break;
            growth -= cost;
            ++rank;
        }
        return { rank, growth };
    }

    // Growth from feeding a weapon: grows with its item level, scaled by its quality.
    inline uint32_t FeedGrowth(uint32_t itemLevel, uint32_t base, float perItemLevel, float qualityPct)
    {
        double growth = (double(base) + double(perItemLevel) * double(itemLevel)) * double(qualityPct) / 100.0;
        if (growth <= 0.0)
            return 0;
        return growth >= double(UINT32_MAX) ? UINT32_MAX : uint32_t(std::lround(growth));
    }

    // Saturating add, for Growth and essence counts.
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
}

#endif
