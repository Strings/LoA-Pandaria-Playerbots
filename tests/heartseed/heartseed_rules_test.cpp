#include "TestMain.h"
#include "HeartseedRules.h"

using namespace Heartseed;

namespace
{
    void EffectiveRankHeldToCap()
    {
        // One seed per account: an alt or a twink uses the shared rank only up to its own cap.
        UMP_CHECK(EffectiveRank(64, 20) == 20);
        UMP_CHECK(EffectiveRank(12, 59) == 12);
        UMP_CHECK(EffectiveRank(30, 30) == 30);
        UMP_CHECK(EffectiveRank(64, RankCap(1, 1.0f, 0, 5)) == 1);
    }

    void DefaultUnlockLevels()
    {
        UMP_CHECK(MajorTierLevel(0) == 10 && MajorTierLevel(5) == 85 && MajorTierLevel(MaxMajorTiers) == 0);
        UMP_CHECK(MajorTierForLevel(25) == 1 && MajorTierForLevel(26) == MaxMajorTiers);
        UMP_CHECK(!MajorTierOpen(2, 39) && MajorTierOpen(2, 40));
        UMP_CHECK(EssenceSlotsForLevel(9) == 0 && EssenceSlotsForLevel(40) == 2 && EssenceSlotsForLevel(90) == 3);
    }

    void ConfiguredUnlockLevels()
    {
        UnlockLevels saved = Unlocks();
        UMP_CHECK(SetLevels(Unlocks().MajorTiers, { 5, 15, 30, 45, 60, 75 }));
        UMP_CHECK(SetLevels(Unlocks().EssenceSlots, { 1, 1, 50 }));   // equal levels are allowed
        UMP_CHECK(MajorTierForLevel(15) == 1 && MajorTierOpen(0, 5) && !MajorTierOpen(5, 74));
        UMP_CHECK(EssenceSlotsForLevel(1) == 2 && FreeEssenceSlot({ 1 }, 1) == 2 && FreeEssenceSlot({ 1, 2 }, 49) == 0);
        Unlocks() = saved;
    }

    void RejectedUnlockLevels()
    {
        UnlockLevels saved = Unlocks();
        std::array<uint8_t, MaxEssenceSlots> before = Unlocks().EssenceSlots;
        UMP_CHECK(!SetLevels(Unlocks().EssenceSlots, { 10, 40 }));        // wrong count
        UMP_CHECK(!SetLevels(Unlocks().EssenceSlots, { 40, 10, 70 }));    // not ascending
        UMP_CHECK(!SetLevels(Unlocks().EssenceSlots, { 0, 10, 70 }));     // level 0
        UMP_CHECK(!SetLevels(Unlocks().EssenceSlots, { 10, 40, 256 }));   // past a level byte
        UMP_CHECK(Unlocks().EssenceSlots == before);
        Unlocks() = saved;
    }
}

void RunTests()
{
    EffectiveRankHeldToCap();
    DefaultUnlockLevels();
    ConfiguredUnlockLevels();
    RejectedUnlockLevels();
}
