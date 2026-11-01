#ifndef ULTIMATEMOP_RAID_RESET_POLICY_H
#define ULTIMATEMOP_RAID_RESET_POLICY_H
#include <cstdint>
namespace RaidResetPolicy
{
    constexpr std::uint32_t Period(std::int32_t configured)
    {
        return configured > 0 ? static_cast<std::uint32_t>(configured) : 172800;
    }
    constexpr bool Complete(std::uint32_t completed, std::uint32_t expected, bool lfgFinal)
    {
        return completed && (lfgFinal || (expected && (completed & expected) == expected));
    }
    constexpr bool Preserve(std::uint32_t completed, bool finished)
    {
        return completed && !finished;
    }
    constexpr bool MayAbandon(bool bound, bool grouped, bool leader, bool bot)
    {
        return bound && !bot && (!grouped || leader);
    }
    constexpr std::uint64_t NextReset(std::uint64_t current, std::uint64_t now, std::uint32_t period)
    {
        if (!current || current > now + period)
            return now + period;
        return current > now ? current : current + ((now - current) / period + 1) * period;
    }
}
#endif
