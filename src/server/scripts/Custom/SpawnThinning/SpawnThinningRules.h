/*
 * Spawn thinning rules (T-081): pure logic, no server dependencies, unit-tested in tests/spawn_thinning/.
 */

#ifndef SPAWN_THINNING_RULES_H
#define SPAWN_THINNING_RULES_H

#include <array>
#include <cmath>
#include <cstdint>
#include <map>
#include <tuple>
#include <utility>
#include <vector>

namespace SpawnThinning
{
    // Spawns only crowd each other inside one group (map and phase, plus the entry when thinning same-entry only).
    using Group = std::array<uint32_t, 5>;

    struct Spawn
    {
        uint64_t Id = 0;
        Group SpawnGroup{};
        float X = 0.0f;
        float Y = 0.0f;
    };

    // Greedy in the given order: a spawn is thinned when a spawn kept before it, in the same group, is closer than
    // radius. Callers pass spawns sorted by id, so the oldest spawn of a crowd stays. Returns the thinned ids.
    inline std::vector<uint64_t> SelectThinned(std::vector<Spawn> const& spawns, float radius)
    {
        std::vector<uint64_t> thinned;
        if (radius <= 0.0f)
            return thinned;

        // Cells as wide as the radius: a crowding spawn is always in the same or a neighbouring cell.
        std::map<std::tuple<Group, int32_t, int32_t>, std::vector<std::pair<float, float>>> cells;
        float const radiusSq = radius * radius;
        for (Spawn const& spawn : spawns)
        {
            int32_t cx = int32_t(std::floor(spawn.X / radius));
            int32_t cy = int32_t(std::floor(spawn.Y / radius));
            bool crowded = false;
            for (int32_t dx = -1; dx <= 1 && !crowded; ++dx)
                for (int32_t dy = -1; dy <= 1 && !crowded; ++dy)
                {
                    auto cell = cells.find({ spawn.SpawnGroup, cx + dx, cy + dy });
                    if (cell == cells.end())
                        continue;
                    for (auto const& [x, y] : cell->second)
                    {
                        float ddx = spawn.X - x;
                        float ddy = spawn.Y - y;
                        if (ddx * ddx + ddy * ddy < radiusSq)
                        {
                            crowded = true;
                            break;
                        }
                    }
                }
            if (crowded)
                thinned.push_back(spawn.Id);
            else
                cells[{ spawn.SpawnGroup, cx, cy }].emplace_back(spawn.X, spawn.Y);
        }
        return thinned;
    }
}

#endif
