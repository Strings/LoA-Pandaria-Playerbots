#include "TestMain.h"
#include "MountedSpeed.h"
#include <limits>

void RunTests()
{
    using MountedSpeed::AdjustRate;
    struct RidingTier { float stock; float doubled; };
    RidingTier const tiers[] = { { 1.6f, 2.2f }, { 2.0f, 3.0f }, { 2.5f, 4.0f }, { 3.8f, 6.6f }, { 4.1f, 7.2f } };
    for (RidingTier const& tier : tiers)
    {
        UMP_CHECK(std::abs(AdjustRate(tier.stock, 2.0f) - tier.doubled) < 0.00001f);
        UMP_CHECK(AdjustRate(tier.stock, 1.0f) == tier.stock);
    }
    UMP_CHECK(AdjustRate(2.0f, 0.5f) == 1.5f);
    UMP_CHECK(AdjustRate(1.0f, 2.0f) == 1.0f);
    UMP_CHECK(AdjustRate(0.5f, 2.0f) == 0.5f);
    UMP_CHECK(MountedSpeed::ValidMultiplier(1.0f) == 1.0f);
    UMP_CHECK(MountedSpeed::ValidMultiplier(3.0f) == 3.0f);
    UMP_CHECK(MountedSpeed::ValidMultiplier(0.0f) == 2.0f);
    UMP_CHECK(MountedSpeed::ValidMultiplier(-1.0f) == 2.0f);
    UMP_CHECK(MountedSpeed::ValidMultiplier(std::numeric_limits<float>::infinity()) == 2.0f);
    UMP_CHECK(MountedSpeed::ValidMultiplier(std::numeric_limits<float>::quiet_NaN()) == 2.0f);
}
