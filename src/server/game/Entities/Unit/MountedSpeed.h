#ifndef ULTIMATEMOP_MOUNTED_SPEED_H
#define ULTIMATEMOP_MOUNTED_SPEED_H

#include <cmath>

namespace MountedSpeed
{
    void Load();
    float Multiplier();

    inline float ValidMultiplier(float value)
    {
        return std::isfinite(value) && value > 0.0f ? value : 2.0f;
    }

    constexpr float AdjustRate(float rate, float multiplier)
    {
        // Riding percentages describe the bonus above ordinary movement.
        return rate > 1.0f ? 1.0f + (rate - 1.0f) * multiplier : rate;
    }
}

#endif
