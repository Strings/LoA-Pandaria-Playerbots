#include "TestMain.h"
#include "TimeValue.h"

void RunTests()
{
    UMP_CHECK(!TimeValue::zero());
    TimeValue elapsed(Microseconds(1999));
    UMP_CHECK(elapsed.ToMilliseconds() == 1);
    UMP_CHECK(elapsed.ToMicroseconds() == 1000);
    UMP_CHECK(TimeValue(Microseconds(-1999)).ToMilliseconds() == -1);

    TimeValue deadline(Seconds(2));
    deadline += TimeValue(Milliseconds(500));
    UMP_CHECK(deadline.ToMilliseconds() == 2500);
    UMP_CHECK(deadline.ToSeconds() == 2);
    UMP_CHECK(deadline - TimeValue(Seconds(3)) == TimeValue(Milliseconds(-500)));
    UMP_CHECK(TimeValue(Seconds(1)) < deadline);
    deadline -= TimeValue(Milliseconds(2500));
    UMP_CHECK(deadline == TimeValue::zero());
}
