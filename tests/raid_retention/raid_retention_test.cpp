#include "TestMain.h"
#include "RaidResetPolicy.h"
void RunTests()
{
    using namespace RaidResetPolicy;
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
