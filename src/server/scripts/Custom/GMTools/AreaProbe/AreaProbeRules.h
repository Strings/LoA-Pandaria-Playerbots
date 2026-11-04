/*
 * Area probe geometry (see README.md): where the probes stand, and what their hits and misses say about a spell's
 * area. Server-independent, so the fitting is unit tested.
 */

#ifndef AREA_PROBE_RULES_H
#define AREA_PROBE_RULES_H

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <map>
#include <vector>

namespace AreaProbe
{
    constexpr float Pi = 3.14159265358979f;
    constexpr float Unbounded = std::numeric_limits<float>::infinity();

    // An angle in [-pi, pi).
    inline float Normalize(float angle)
    {
        angle = std::fmod(angle + Pi, 2.0f * Pi);
        if (angle < 0.0f)
            angle += 2.0f * Pi;
        return angle - Pi;
    }

    // A probe's place relative to the caster: yards out, radians from its facing (positive to the left, the way
    // orientation turns), and the spoke it stands on.
    struct Slot
    {
        float Distance;
        float Angle;
        uint32_t Ray;
    };

    // Rings every `step` yards out to rings * step, on `rays` spokes spread evenly over `arc` radians centered on the
    // facing; an arc of 2 pi or more is the full circle.
    inline std::vector<Slot> Layout(float step, uint32_t rings, uint32_t rays, float arc)
    {
        std::vector<Slot> slots;
        if (step <= 0.0f || !rings || !rays)
            return slots;
        bool full = arc >= 2.0f * Pi - 1e-4f;
        for (uint32_t ray = 0; ray < rays; ++ray)
        {
            float angle = 0.0f;
            if (full)
                angle = 2.0f * Pi * ray / rays;
            else if (rays > 1)
                angle = -arc / 2.0f + arc * ray / (rays - 1);
            for (uint32_t ring = 1; ring <= rings; ++ring)
                slots.push_back({ step * ring, Normalize(angle), ray });
        }
        return slots;
    }

    // A point in the caster's frame: X yards ahead, Y yards to the left.
    struct Point
    {
        float X;
        float Y;
    };

    inline Point ToLocal(float casterX, float casterY, float casterO, float x, float y)
    {
        float dx = x - casterX, dy = y - casterY;
        float c = std::cos(casterO), s = std::sin(casterO);
        return { dx * c + dy * s, -dx * s + dy * c };
    }

    struct Sample
    {
        Point At;
        uint32_t Ray;
        bool Hit;
    };

    // The edge lies between Low (the farthest probe inside) and High (the nearest probe outside it); High is
    // Unbounded when nothing beyond was missed.
    struct Range
    {
        float Low = 0.0f;
        float High = Unbounded;
    };

    struct Fit
    {
        uint32_t Hits = 0;
        uint32_t Misses = 0;
        uint32_t Rays = 0;
        uint32_t RaysHit = 0;
        bool FullCircle = false;    // every spoke was hit beyond half the reach
        bool Gaps = false;          // a spoke missed closer in than its farthest hit
        float Center = 0.0f;        // direction of the spokes hit beyond half the reach, radians from facing
        Range HalfAngle;            // radians either side of Center, from the spokes hit beyond half the reach
        float CloseHalfAngle = 0.0f;    // the widest hit spoke at any distance: wider when close targets count
        float CloseReach = 0.0f;        // how far out the hits wider than HalfAngle go
        Range Reach;                // yards from the caster
        Point CircleCenter{ 0.0f, 0.0f };   // centroid of the hits, for areas not centered on the caster
        Range Radius;               // around CircleCenter
    };

    inline float Length(Point p) { return std::sqrt(p.X * p.X + p.Y * p.Y); }

    inline Fit Analyze(std::vector<Sample> const& samples)
    {
        Fit fit;
        std::map<uint32_t, std::vector<Sample const*>> rays;
        float sumX = 0.0f, sumY = 0.0f;
        for (Sample const& sample : samples)
        {
            rays[sample.Ray].push_back(&sample);
            if (sample.Hit)
            {
                ++fit.Hits;
                sumX += sample.At.X;
                sumY += sample.At.Y;
            }
            else
                ++fit.Misses;
        }
        fit.Rays = uint32_t(rays.size());
        if (!fit.Hits)
            return fit;

        struct Spoke
        {
            float Angle;
            float Farthest;     // -1 when nothing on it was hit
        };
        std::vector<Spoke> spokes;
        for (auto const& [ray, members] : rays)
        {
            float x = 0.0f, y = 0.0f, farthest = -1.0f;
            for (Sample const* sample : members)
            {
                x += sample->At.X;
                y += sample->At.Y;
                if (sample->Hit)
                    farthest = std::max(farthest, Length(sample->At));
            }
            spokes.push_back({ std::atan2(y, x), farthest });
            if (farthest < 0.0f)
                continue;

            ++fit.RaysHit;
            fit.Reach.Low = std::max(fit.Reach.Low, farthest);
            for (Sample const* sample : members)
                if (!sample->Hit && Length(sample->At) < farthest)
                    fit.Gaps = true;
        }
        // The angle comes from spokes hit beyond half the reach: close in, a target's size (and some cores' melee
        // allowance) widens it.
        float farFrom = fit.Reach.Low / 2.0f;
        float dirX = 0.0f, dirY = 0.0f;
        uint32_t farSpokes = 0;
        for (Spoke const& spoke : spokes)
            if (spoke.Farthest >= farFrom)
            {
                ++farSpokes;
                dirX += std::cos(spoke.Angle);
                dirY += std::sin(spoke.Angle);
            }

        fit.FullCircle = farSpokes == fit.Rays;
        if (fit.FullCircle)
            fit.HalfAngle = { Pi, Pi };
        else
        {
            fit.Center = std::atan2(dirY, dirX);
            fit.HalfAngle.High = Pi;
            for (Spoke const& spoke : spokes)
                if (spoke.Farthest >= farFrom)
                    fit.HalfAngle.Low = std::max(fit.HalfAngle.Low, std::fabs(Normalize(spoke.Angle - fit.Center)));
            for (Spoke const& spoke : spokes)
            {
                float offset = std::fabs(Normalize(spoke.Angle - fit.Center));
                if (spoke.Farthest < farFrom && offset > fit.HalfAngle.Low)
                    fit.HalfAngle.High = std::min(fit.HalfAngle.High, offset);
            }
        }
        // Only misses past the farthest hit, on spokes inside the angle, bound the reach; the others missed for their
        // angle.
        for (Sample const& sample : samples)
        {
            float distance = Length(sample.At);
            float offset = std::fabs(Normalize(std::atan2(sample.At.Y, sample.At.X) - fit.Center));
            if (!sample.Hit && distance > fit.Reach.Low + 1e-3f && offset <= fit.HalfAngle.Low + 1e-3f)
                fit.Reach.High = std::min(fit.Reach.High, distance);
        }

        fit.CloseHalfAngle = fit.HalfAngle.Low;
        for (Spoke const& spoke : spokes)
        {
            float offset = std::fabs(Normalize(spoke.Angle - fit.Center));
            if (spoke.Farthest >= 0.0f && offset > fit.HalfAngle.Low + 1e-4f)
            {
                fit.CloseHalfAngle = std::max(fit.CloseHalfAngle, offset);
                fit.CloseReach = std::max(fit.CloseReach, spoke.Farthest);
            }
        }

        fit.CircleCenter = { sumX / fit.Hits, sumY / fit.Hits };
        for (Sample const& sample : samples)
        {
            float distance = Length({ sample.At.X - fit.CircleCenter.X, sample.At.Y - fit.CircleCenter.Y });
            if (sample.Hit)
                fit.Radius.Low = std::max(fit.Radius.Low, distance);
        }
        for (Sample const& sample : samples)
        {
            float distance = Length({ sample.At.X - fit.CircleCenter.X, sample.At.Y - fit.CircleCenter.Y });
            if (!sample.Hit && distance > fit.Radius.Low)
                fit.Radius.High = std::min(fit.Radius.High, distance);
        }
        return fit;
    }
}

#endif
