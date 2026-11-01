#ifndef ULTIMATEMOP_SOLO_DUNGEON_POLICY_H
#define ULTIMATEMOP_SOLO_DUNGEON_POLICY_H
#include <cstdint>
namespace LegacyDungeons
{
    enum class Decision { Native, Solo, Deferred, RequireSolo, RequireSpecific, UnknownCap };
    constexpr std::uint32_t OriginalCap(std::uint32_t minimum, std::uint32_t maximum) { return maximum ? maximum : minimum; }
    constexpr std::uint32_t Margin(std::int32_t configured) { return configured < 1 ? 1u : std::uint32_t(configured); }
    constexpr Decision Decide(bool enabled, bool bot, bool fiveMan, bool specific, bool grouped,
        std::uint32_t level, std::uint32_t cap, std::uint32_t margin, bool force = false)
    {
        if (!enabled || bot || !fiveMan)
            return Decision::Native;
        if (!cap)
            return force ? Decision::UnknownCap : Decision::Native;
        if (!force && level <= cap)
            return Decision::Native;
        if (!specific)
            return Decision::RequireSpecific;
        if (grouped)
            return Decision::RequireSolo;
        return force || level - cap >= margin ? Decision::Solo : Decision::Deferred;
    }
}
#endif
