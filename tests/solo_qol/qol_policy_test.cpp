#include "TestMain.h"
#include "QoLPolicy.h"
void RunTests()
{
    using namespace QoLPolicy;
    UMP_CHECK(!CooldownMillis(-1));
    UMP_CHECK(CooldownMillis(0) == 0);
    UMP_CHECK(CooldownMillis(300) == 300000);
    UMP_CHECK(CooldownMillis(INT64_MAX) == 2147483000);
    UMP_CHECK(WantPerk(true, false, 0, 25));
    UMP_CHECK(!WantPerk(false, false, 0, 1));
    UMP_CHECK(!WantPerk(false, true, 1, 25));
    UMP_CHECK(WantPerk(false, true, 25, 25));
    UMP_CHECK(ServiceFlag(1) == 1 && ServiceFlag(2) == 8);
    UMP_CHECK(ServiceFlag(3) == 128 && ServiceFlag(4) == 64);
    UMP_CHECK(ServiceFlag(0) == 0 && ServiceFlag(999) == 0);
    UMP_CHECK(CanUseServices(true, false, false, false));
    UMP_CHECK(!CanUseServices(false, false, false, false));
    UMP_CHECK(!CanUseServices(true, true, false, false));
    UMP_CHECK(!CanUseServices(true, false, true, false));
    UMP_CHECK(!CanUseServices(true, false, false, true));
}
