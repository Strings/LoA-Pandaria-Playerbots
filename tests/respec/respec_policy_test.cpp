#include "TestMain.h"
#include "RespecPolicy.h"
void RunTests()
{
    using RespecPolicy::ShouldUnlock;
    UMP_CHECK(!ShouldUnlock(9, 10, 1, true, false));
    UMP_CHECK(ShouldUnlock(10, 10, 1, true, false));
    UMP_CHECK(ShouldUnlock(90, 10, 1, true, false));
    UMP_CHECK(!ShouldUnlock(10, 20, 1, true, false));
    UMP_CHECK(ShouldUnlock(20, 20, 1, true, false));
    UMP_CHECK(!ShouldUnlock(9, 0, 1, true, false));
    UMP_CHECK(ShouldUnlock(10, 0, 1, true, false));
    UMP_CHECK(!ShouldUnlock(90, 10, 2, true, false));
    UMP_CHECK(!ShouldUnlock(90, 10, 1, false, false));
    UMP_CHECK(!ShouldUnlock(90, 10, 1, true, true));
}
