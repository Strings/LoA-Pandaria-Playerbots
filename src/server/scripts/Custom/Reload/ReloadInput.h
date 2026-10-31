#ifndef ULTIMATEMOP_RELOAD_INPUT_H
#define ULTIMATEMOP_RELOAD_INPUT_H

#include <charconv>
#include <cstdint>
#include <string_view>

namespace CustomReload
{
    inline bool ParseSeed(std::string_view text, std::uint32_t& seed)
    {
        if (text.empty())
            return false;
        std::uint32_t parsed = 0;
        auto result = std::from_chars(text.data(), text.data() + text.size(), parsed);
        if (result.ec != std::errc() || result.ptr != text.data() + text.size())
            return false;
        seed = parsed;
        return true;
    }
}

#endif
