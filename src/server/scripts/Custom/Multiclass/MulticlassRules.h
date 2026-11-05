/*
 * Multi-classing: pure rules, no server types, so they are unit-tested (tests/multiclass).
 */

#ifndef ULTIMATEMOP_MULTICLASS_RULES_H
#define ULTIMATEMOP_MULTICLASS_RULES_H

#include <algorithm>
#include <cstdint>

namespace Multiclass
{
    // A second class's level and experience into that level.
    struct Progress
    {
        uint8_t Level = 1;
        uint32_t Xp = 0;
    };

    // Experience for one second-class level: a share of what the character's own next level costs (at the top
    // level, the level below it), so a second class takes about as long at 90 as at 40.
    inline uint32_t XpPerLevel(uint32_t characterLevelXp, uint32_t xpPct)
    {
        uint64_t need = uint64_t(characterLevelXp) * xpPct / 100;
        return uint32_t(std::clamp<uint64_t>(need, 1, UINT32_MAX));
    }

    // Adds experience; levels up while it lasts, never past maxLevel (experience stops there).
    inline Progress AddXp(Progress p, uint64_t gain, uint32_t perLevel, uint8_t maxLevel)
    {
        perLevel = std::max<uint32_t>(perLevel, 1);
        maxLevel = std::max<uint8_t>(maxLevel, 1);
        if (p.Level >= maxLevel)
            return { maxLevel, 0 };
        uint64_t xp = uint64_t(p.Xp) + gain;
        while (xp >= perLevel && p.Level < maxLevel)
        {
            xp -= perLevel;
            ++p.Level;
        }
        p.Xp = p.Level >= maxLevel ? 0 : uint32_t(xp);
        return p;
    }

    // A rule's internal cooldown: ready when it never fired or the cooldown has passed (wrap-safe ms clock).
    inline bool RuleReady(uint32_t lastFired, uint32_t now, uint32_t cooldownMs)
    {
        return !lastFired || !cooldownMs || uint32_t(now - lastFired) >= cooldownMs;
    }
}

#endif
