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
    UMP_CHECK(MountedSpeed::GhostRate(1.5f, 2.0f) == 2.0f);   // the Ghost aura's 150% is raised
    UMP_CHECK(MountedSpeed::GhostRate(2.5f, 2.0f) == 2.5f);   // a faster effect keeps its speed
    UMP_CHECK(MountedSpeed::GhostRate(1.5f, 0.0f) == 1.5f);   // 0 leaves stock ghost speed
    // Master Riding at 2.4 is 1 + 3.1 * 2.4 = 8.44 times 7 yd/s; twice that is about 118 yd/s.
    UMP_CHECK(std::fabs(MountedSpeed::TaxiSpeed(32.0f, 7.0f, 2.4f, 2.0f) - 118.16f) < 0.01f);
    UMP_CHECK(MountedSpeed::TaxiSpeed(32.0f, 7.0f, 2.4f, 0.0f) == 32.0f);   // 0 keeps stock
    UMP_CHECK(MountedSpeed::TaxiSpeed(32.0f, 7.0f, 1.0f, 0.5f) == 32.0f);   // never slower than stock
    UMP_CHECK(MountedSpeed::ValidMultiplier(0.0f) == 2.4f);
    UMP_CHECK(MountedSpeed::ValidMultiplier(-1.0f) == 2.4f);
    UMP_CHECK(MountedSpeed::ValidMultiplier(std::numeric_limits<float>::infinity()) == 2.4f);
    UMP_CHECK(MountedSpeed::ValidMultiplier(std::numeric_limits<float>::quiet_NaN()) == 2.4f);
}
