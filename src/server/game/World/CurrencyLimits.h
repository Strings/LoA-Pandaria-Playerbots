#ifndef ULTIMATEMOP_CURRENCY_LIMITS_H
#define ULTIMATEMOP_CURRENCY_LIMITS_H

#include <algorithm>
#include <cstdint>

namespace SoloConfig
{
    constexpr std::int32_t LimitCurrencyAward(std::int32_t amount, std::uint32_t cap, std::uint32_t earned)
    {
        if (amount <= 0 || !cap)
            return amount;
        // Lowering or re-enabling a cap must never turn a reward into a deduction.
        std::uint32_t remaining = earned < cap ? cap - earned : 0;
        return static_cast<std::int32_t>(std::min(static_cast<std::uint32_t>(amount), remaining));
    }
}

#endif
