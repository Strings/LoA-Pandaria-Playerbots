/*
 * Heartseed rules that need nothing from the server: stage by level, dominant path, slot counts.
 * Kept free of game headers so the unit test framework (T-008) can test them directly.
 */

#ifndef HEARTSEED_RULES_H
#define HEARTSEED_RULES_H

#include <array>
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
}

#endif
