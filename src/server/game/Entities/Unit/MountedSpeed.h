#ifndef ULTIMATEMOP_MOUNTED_SPEED_H
#define ULTIMATEMOP_MOUNTED_SPEED_H

#include <cmath>

namespace MountedSpeed
{
    void Load();
    float Multiplier();
    float GhostSpeed();
    float TaxiFactor();

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

    // Master Riding's 310% bonus: the fastest flying a player learns.
    constexpr float TopFlightRate = 4.1f;
    // The core's stock flight-path speed, yards per second.
    constexpr float StockTaxiSpeed = 32.0f;
    // The 5.4.8 client flies a flying spline no faster than this, whatever the packet's duration says (the same
    // limit MoveSplineInit applies). A faster flight path ends on the server first: it dismounts the player while
    // the client is still flying (T-087).
    constexpr float ClientFlyingSplineCap = 50.0f;

    // Flight-path speed in yards per second: `factor` times the fastest player flying (Master Riding through the
    // riding multiplier), never slower than the stock taxi speed nor faster than the client's spline cap; a factor
    // of 0 keeps stock.
    constexpr float TaxiSpeed(float stock, float baseFlight, float multiplier, float factor)
    {
        return factor * baseFlight * AdjustRate(TopFlightRate, multiplier) <= stock ? stock
            : factor * baseFlight * AdjustRate(TopFlightRate, multiplier) >= ClientFlyingSplineCap ? ClientFlyingSplineCap
            : factor * baseFlight * AdjustRate(TopFlightRate, multiplier);
    }
}

#endif
