#include "TestMain.h"
#include "CurrencyLimits.h"
#include <limits>

void RunTests()
{
    using SoloConfig::LimitCurrencyAward;
    UMP_CHECK(LimitCurrencyAward(150000, 0, 200000) == 150000);
    UMP_CHECK(LimitCurrencyAward(500000, 100000, 0) == 100000);
    UMP_CHECK(LimitCurrencyAward(50000, 100000, 75000) == 25000);
    UMP_CHECK(LimitCurrencyAward(1, 100000, 100000) == 0);
    UMP_CHECK(LimitCurrencyAward(1, 100000, 300099) == 0);
    UMP_CHECK(LimitCurrencyAward(1, 135000, 150000) == 0);
    UMP_CHECK(LimitCurrencyAward(1, 165000, 350000) == 0);
    UMP_CHECK(LimitCurrencyAward(-1000, 100000, 150000) == -1000);
    UMP_CHECK(LimitCurrencyAward(0, 100000, 150000) == 0);
    UMP_CHECK(LimitCurrencyAward(std::numeric_limits<std::int32_t>::max(),
        std::numeric_limits<std::uint32_t>::max(), 0) == std::numeric_limits<std::int32_t>::max());
    UMP_CHECK(LimitCurrencyAward(10, std::numeric_limits<std::uint32_t>::max(),
        std::numeric_limits<std::uint32_t>::max() - 1) == 1);
}
