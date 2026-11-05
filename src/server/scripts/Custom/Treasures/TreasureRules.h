/*
 * Treasure placement rules (T-093): pure logic, no server dependencies, unit-tested in tests/treasures/.
 * The server side measures the world (heights, vmap rays, navmesh path) and these rules turn the measurements
 * into a verdict, so the same rules can later serve the offline placement tool (DESIGN.md, World placement tool).
 */

#ifndef TREASURE_RULES_H
#define TREASURE_RULES_H

#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace Treasures
{
    // Steepest gradient in degrees from four neighbour heights sampled `step` yards from the centre.
    inline float SlopeDegrees(float east, float west, float north, float south, float step)
    {
        if (step <= 0.0f)
            return 0.0f;
        float dx = (east - west) / (2.0f * step);
        float dy = (north - south) / (2.0f * step);
        return std::atan(std::sqrt(dx * dx + dy * dy)) * 57.29578f;
    }

    enum class Sky
    {
        Covered, // every upward ray hits something, or a WMO marks the spot indoors
        Partial, // some rays hit: an overhang or a tree, landable from some angles
        Open     // nothing above: visible and landable from the air
    };

    inline Sky SkyFromRays(int blocked, int total, bool indoors)
    {
        if (indoors || (total > 0 && blocked >= total))
            return Sky::Covered;
        return blocked > 0 ? Sky::Partial : Sky::Open;
    }

    inline char const* SkyName(Sky sky)
    {
        switch (sky)
        {
            case Sky::Covered: return "covered";
            case Sky::Partial: return "partly covered";
            default:           return "open sky";
        }
    }

    struct Limits
    {
        float MaxGroundOffset = 3.0f; // candidate z may sit this far from the snapped ground
        float MaxSlope = 40.0f;       // degrees
        float MinSpacing = 60.0f;     // yards to the nearest other treasure
        float MaxPathFactor = 4.0f;   // walking path may be this many times the straight distance
        bool RequireCover = false;    // open sky fails instead of warning (DESIGN.md, Flying rule)
    };

    struct Measurement
    {
        bool GroundFound = false;
        float GroundOffset = 0.0f;   // |candidate z - snapped ground|
        bool Submerged = false;      // under water or magma at the snapped ground
        float Slope = 0.0f;
        bool Reachable = false;      // complete navmesh path from the access point
        float PathLength = 0.0f;
        float StraightDistance = 0.0f;
        bool LineOfSight = false;    // from the access point (information only: hidden spots may lack it)
        Sky SkyState = Sky::Open;
        float NearestTreasure = -1.0f; // < 0 when there is no other treasure on the map
    };

    struct Verdict
    {
        std::vector<std::string> Failures;
        std::vector<std::string> Warnings;
        int Score = 0; // higher is better; only compares candidates that pass
        bool Passed() const { return Failures.empty(); }
    };

    inline Verdict Judge(Measurement const& m, Limits const& limits)
    {
        Verdict v;
        if (!m.GroundFound)
            v.Failures.push_back("no ground");
        else if (m.GroundOffset > limits.MaxGroundOffset)
            v.Failures.push_back("floating or buried");
        if (m.Submerged)
            v.Failures.push_back("under liquid");
        if (m.Slope > limits.MaxSlope)
            v.Failures.push_back("too steep");
        if (!m.Reachable)
            v.Failures.push_back("unreachable on foot");
        else if (m.StraightDistance > 1.0f && m.PathLength > m.StraightDistance * limits.MaxPathFactor)
            v.Warnings.push_back("long detour");
        if (m.NearestTreasure >= 0.0f && m.NearestTreasure < limits.MinSpacing)
            v.Failures.push_back("too close to another treasure");
        if (m.SkyState == Sky::Open)
            (limits.RequireCover ? v.Failures : v.Warnings).push_back("open sky");

        // Cover is the point of a treasure nook; flat ground and a short walk help.
        v.Score = (m.SkyState == Sky::Covered ? 50 : m.SkyState == Sky::Partial ? 25 : 0)
            + int(std::fmax(0.0f, 20.0f - m.Slope / 2.0f))
            + (m.LineOfSight ? 0 : 10)
            - int(v.Warnings.size()) * 5;
        return v;
    }

    // Offsets to try when nudging a failing candidate: rings of 8 points every `step` yards out to `radius`,
    // nearest first, so the nudge moves the spot as little as possible.
    inline std::vector<std::pair<float, float>> NudgeOffsets(float radius, float step)
    {
        std::vector<std::pair<float, float>> offsets;
        if (step <= 0.0f)
            return offsets;
        for (float r = step; r <= radius + 0.001f; r += step)
            for (int i = 0; i < 8; ++i)
            {
                float angle = float(i) * 0.7853982f;
                offsets.emplace_back(r * std::cos(angle), r * std::sin(angle));
            }
        return offsets;
    }

    inline std::string Join(std::vector<std::string> const& parts)
    {
        std::string out;
        for (std::string const& part : parts)
            out += (out.empty() ? "" : ", ") + part;
        return out;
    }
}

#endif
