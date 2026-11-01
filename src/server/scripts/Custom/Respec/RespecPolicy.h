#ifndef ULTIMATEMOP_RESPEC_POLICY_H
#define ULTIMATEMOP_RESPEC_POLICY_H
#include <cstdint>
namespace RespecPolicy
{
    constexpr bool ShouldUnlock(std::uint32_t level, std::uint32_t configuredLevel, std::uint32_t specs, bool enabled, bool bot)
    {
        return enabled && !bot && specs < 2 && level >= (configuredLevel < 10 ? 10 : configuredLevel);
    }
}
#endif
