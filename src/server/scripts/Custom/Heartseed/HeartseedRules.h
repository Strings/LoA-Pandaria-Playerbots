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
}

#endif
