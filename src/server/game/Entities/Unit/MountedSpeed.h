#ifndef ULTIMATEMOP_MOUNTED_SPEED_H
#define ULTIMATEMOP_MOUNTED_SPEED_H

#include <cmath>

namespace MountedSpeed
{
    void Load();
    float Multiplier();
    float GhostSpeed();

    inline float ValidMultiplier(float value)
    {
        return std::isfinite(value) && value > 0.0f ? value : 2.4f;
    }

    // A ghost runs and swims at least this fast (Death.GhostSpeed); faster effects keep their speed.
    constexpr float GhostRate(float rate, float ghostSpeed)
    {
        return rate < ghostSpeed ? ghostSpeed : rate;
    }

    constexpr float AdjustRate(float rate, float multiplier)
    {
        // Riding percentages describe the bonus above ordinary movement.
        return rate > 1.0f ? 1.0f + (rate - 1.0f) * multiplier : rate;
    }
}

#endif
