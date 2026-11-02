#ifndef ULTIMATEMOP_SOLO_QOL_POLICY_H
#define ULTIMATEMOP_SOLO_QOL_POLICY_H
#include <cstdint>
#include <limits>
#include <optional>
namespace QoLPolicy
{
    constexpr std::optional<std::int32_t> CooldownMillis(std::int64_t seconds)
    {
        if (seconds < 0)
            return std::nullopt;
        constexpr std::int64_t maxSeconds = std::numeric_limits<std::int32_t>::max() / 1000;
        return std::int32_t((seconds > maxSeconds ? maxSeconds : seconds) * 1000);
    }
    constexpr bool WantPerk(bool everyone, bool hasGuild, std::uint32_t level, std::uint32_t required)
    {
        return everyone || (hasGuild && level >= required);
    }
    constexpr std::uint32_t ServiceFlag(std::uint32_t action)
    {
        return action == 1 ? 1 : action == 2 ? 8 : action == 3 ? 128 : action == 4 ? 64 : 0;
    }
    constexpr bool CanUseServices(bool enabled, bool bot, bool neutral, bool combat)
    {
        return enabled && !bot && !neutral && !combat;
    }
}
#endif
