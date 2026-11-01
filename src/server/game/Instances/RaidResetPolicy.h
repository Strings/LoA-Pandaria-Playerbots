#ifndef ULTIMATEMOP_RAID_RESET_POLICY_H
#define ULTIMATEMOP_RAID_RESET_POLICY_H
#include <cstdint>
namespace RaidResetPolicy
{
    constexpr std::uint32_t PartialMarker = ~std::uint32_t(0);
    constexpr bool Finished(std::uint32_t marker)
    {
        return marker && marker != PartialMarker;
    }
    constexpr bool HasProgress(std::uint32_t completed, std::uint32_t marker)
    {
        return completed || marker == PartialMarker;
    }
    constexpr std::uint32_t AllBossMask(std::uint32_t count)
    {
        return count > 32 ? 0 : count == 32 ? ~std::uint32_t(0) : (std::uint32_t(1) << count) - 1;
    }
    // Existing MoP script boss indices and LFGDungeons.dbc wing IDs.
    // Exclude off-wing DONE states and the heroic-only Ra-den encounter.
    constexpr std::uint32_t ScriptMask(std::uint32_t map, std::uint32_t count, bool heroic, std::uint32_t wing)
    {
        if (map == 1008 && wing == 527) return 7;
        if (map == 1008 && wing == 528) return 56;
        if (map == 1009 && wing == 529) return 7;
        if (map == 1009 && wing == 530) return 56;
        if (map == 996 && wing == 526) return 15;
        if (map == 1098 && wing == 610) return 7;
        if (map == 1098 && wing == 611) return 56;
        if (map == 1098 && wing == 612) return 448;
        if (map == 1098 && wing == 613) return 3584;
        if (map == 1136 && (wing == 716 || wing == 726)) return 15;
        if (map == 1136 && (wing == 717 || wing == 728)) return 240;
        if (map == 1136 && (wing == 724 || wing == 729)) return 1792;
        if (map == 1136 && (wing == 725 || wing == 730)) return 14336;
        if (map == 1098 && !heroic && count == 13) return 4095;
        return AllBossMask(count);
    }
    constexpr std::uint32_t Period(std::int32_t configured)
    {
        return configured > 0 ? static_cast<std::uint32_t>(configured) : 172800;
    }
    constexpr bool Complete(std::uint32_t completed, std::uint32_t expected, bool lfgFinal)
    {
        return completed && (lfgFinal || (expected && (completed & expected) == expected));
    }
    constexpr bool Preserve(std::uint32_t completed, bool finished, bool encounterInProgress = false)
    {
        return !finished && (completed || encounterInProgress);
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
