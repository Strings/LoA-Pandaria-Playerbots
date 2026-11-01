#ifndef ULTIMATEMOP_BINDING_RULES_H
#define ULTIMATEMOP_BINDING_RULES_H

#include <cstdint>

namespace BindingRules
{
    struct Item
    {
        bool Known = true;
        std::uint32_t Class = 0;
        std::uint32_t Bonding = 0;
        std::uint32_t Quality = 0;
        std::uint32_t StartQuest = 0;
        bool ConfiguredProtection = false;
        bool SystemProtection = false;
    };

    constexpr bool Protected(Item const& item)
    {
        return !item.Known || item.Class == 12 || item.Bonding >= 4 || item.Quality == 6 ||
            item.StartQuest != 0 || item.ConfiguredProtection || item.SystemProtection;
    }

    constexpr bool Suppress(bool enabled, Item const& item)
    {
        return enabled && !Protected(item);
    }

    constexpr std::uint32_t TooltipFlags(std::uint32_t flags, bool suppress)
    {
        return suppress ? flags & ~std::uint32_t(0x08000000) : flags;
    }

    constexpr std::uint32_t NextVersion(std::uint32_t previous, std::uint32_t now)
    {
        return previous == UINT32_MAX ? previous : (now > previous ? now : previous + 1);
    }

    constexpr std::uint32_t HotfixBatchRecords(std::uint32_t remaining)
    {
        // Keep each notification below even a 16-bit packet size limit.
        return remaining < 2048 ? remaining : 2048;
    }
}
#endif
