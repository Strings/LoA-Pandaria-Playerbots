#ifndef ULTIMATEMOP_SOLO_DUNGEON_POLICY_H
#define ULTIMATEMOP_SOLO_DUNGEON_POLICY_H
#include <cstdint>
#include <charconv>
#include <optional>
#include <string_view>
namespace LegacyDungeons
{
    inline std::optional<std::uint32_t> DungeonId(std::string_view text)
    {
        auto first = text.find_first_not_of(" \t\r\n");
        if (first == std::string_view::npos)
            return std::nullopt;
        text = text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1);
        std::uint32_t id = 0;
        auto parsed = std::from_chars(text.data(), text.data() + text.size(), id);
        return parsed.ec == std::errc() && parsed.ptr == text.data() + text.size() && id && id <= 0xFFFFFF ? std::optional<std::uint32_t>(id) : std::nullopt;
    }
    enum class Decision { Native, Solo, Deferred, RequireSolo, RequireSpecific, UnknownCap };
    constexpr std::uint32_t OriginalCap(std::uint32_t minimum, std::uint32_t maximum) { return maximum ? maximum : minimum; }
    constexpr std::uint32_t Margin(std::int32_t configured) { return configured < 1 ? 20u : std::uint32_t(configured); }
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
