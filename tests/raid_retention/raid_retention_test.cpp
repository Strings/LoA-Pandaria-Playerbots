#include "TestMain.h"
#include "RaidResetPolicy.h"
void RunTests()
{
    using namespace RaidResetPolicy;
    UMP_CHECK(HoldBossRespawn(true, true, true));
    UMP_CHECK(!HoldBossRespawn(false, true, true));
    UMP_CHECK(!HoldBossRespawn(true, false, true));
    UMP_CHECK(!HoldBossRespawn(true, true, false));
    UMP_CHECK(!Finished(0) && !Finished(PartialMarker) && Finished(123));
    UMP_CHECK(HasProgress(0, PartialMarker) && HasProgress(1, 0) && !HasProgress(0, 0));
    UMP_CHECK(AllBossMask(0) == 0 && AllBossMask(4) == 15);
    UMP_CHECK(AllBossMask(32) == PartialMarker && AllBossMask(33) == 0);
    UMP_CHECK(ScriptMask(1098, 13, false, 0) == 4095);
    UMP_CHECK(ScriptMask(1098, 13, true, 0) == 8191);
    UMP_CHECK(ScriptMask(1008, 6, false, 527) == 7);
    UMP_CHECK(ScriptMask(1008, 6, false, 528) == 56);
    UMP_CHECK(!Complete(56, ScriptMask(1008, 6, false, 527), false));
    UMP_CHECK(Complete(63, ScriptMask(1008, 6, false, 527), false));
    UMP_CHECK(ScriptMask(1009, 6, false, 529) == 7 && ScriptMask(1009, 6, false, 530) == 56);
    UMP_CHECK(ScriptMask(996, 4, false, 526) == 15);
    UMP_CHECK(ScriptMask(1098, 13, false, 610) == 7 && ScriptMask(1098, 13, false, 611) == 56);
    UMP_CHECK(ScriptMask(1098, 13, false, 612) == 448 && ScriptMask(1098, 13, false, 613) == 3584);
    UMP_CHECK(ScriptMask(1136, 14, false, 716) == 15 && ScriptMask(1136, 14, false, 717) == 240);
    UMP_CHECK(ScriptMask(1136, 14, false, 724) == 1792 && ScriptMask(1136, 14, false, 725) == 14336);
    UMP_CHECK(ScriptMask(1136, 14, false, 726) == 15 && ScriptMask(1136, 14, false, 728) == 240);
    UMP_CHECK(ScriptMask(1136, 14, false, 729) == 1792 && ScriptMask(1136, 14, false, 730) == 14336);
    UMP_CHECK(!Preserve(0, false));
    UMP_CHECK(Preserve(0, false, true)); // First boss fight also defers a boundary.
    UMP_CHECK(!Preserve(0, true, true));
    UMP_CHECK(Preserve(1, false));
    UMP_CHECK(Preserve(7, false));
    UMP_CHECK(!Preserve(7, true));
    UMP_CHECK(!Complete(1, 7, false));
    UMP_CHECK(Complete(7, 7, false));
    UMP_CHECK(Complete(15, 7, false));
    UMP_CHECK(!Complete(1, 0, false));
    UMP_CHECK(Complete(1, 7, true)); // LFR wing final, without other wings' bosses.
    UMP_CHECK(!Complete(0, 0, true));
    UMP_CHECK(Period(172800) == 172800);
    UMP_CHECK(Period(20) == 20);
    UMP_CHECK(Period(0) == 172800);
    UMP_CHECK(Period(-1) == 172800);
    UMP_CHECK(NextReset(0, 100, 20) == 120);
    UMP_CHECK(NextReset(115, 100, 20) == 115);
    UMP_CHECK(NextReset(9999, 100, 20) == 120);
    UMP_CHECK(NextReset(100, 100, 20) == 120);
    UMP_CHECK(NextReset(80, 101, 20) == 120);
    UMP_CHECK(NextReset(0, 0, 172800) == 172800);
    UMP_CHECK(MayAbandon(true, false, false, false));
    UMP_CHECK(MayAbandon(true, true, true, false));
    UMP_CHECK(!MayAbandon(true, true, false, false));
    UMP_CHECK(!MayAbandon(false, true, true, false));
    UMP_CHECK(!MayAbandon(true, true, true, true));
}
