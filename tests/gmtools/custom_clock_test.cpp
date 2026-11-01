#include "TestMain.h"
#include "CustomClockRules.h"

using namespace CustomClock;

namespace
{
    void WallTimeByDefault()
    {
        Model m;
        m.Reset(1000);
        UMP_CHECK(m.NowMs(1000) == 1000);
        UMP_CHECK(m.NowMs(61000) == 61000);
        UMP_CHECK(m.AheadMs(61000) == 0);
    }

    void SpeedReanchorsWithoutSkipping()
    {
        Model m;
        m.Reset(0);
        m.SetSpeed(10000, 60.0);            // at real 10s, clock reads 10s
        UMP_CHECK(m.NowMs(10000) == 10000);
        UMP_CHECK(m.NowMs(11000) == 70000); // one real second = one clock minute
        m.SetSpeed(11000, 1.0);             // back to normal keeps the minute gained
        UMP_CHECK(m.NowMs(11000) == 70000);
        UMP_CHECK(m.NowMs(12000) == 71000);
    }

    void JumpAddsOnTopOfSpeed()
    {
        Model m;
        m.Reset(0);
        m.SetSpeed(0, 2.0);
        m.Jump(5000, 3600000);              // clock was 10s; jump an hour
        UMP_CHECK(m.NowMs(5000) == 3610000);
        UMP_CHECK(m.NowMs(6000) == 3612000);
        UMP_CHECK(m.AheadMs(6000) == 3606000);
        m.Reset(6000);
        UMP_CHECK(m.NowMs(7000) == 7000 && m.Speed == 1.0);
    }

    void Durations()
    {
        int64_t s = 0;
        UMP_CHECK(ParseDuration("90", 1000000, s) && s == 90);
        UMP_CHECK(ParseDuration("15m", 1000000, s) && s == 900);
        UMP_CHECK(ParseDuration("1d12h", 1000000, s) && s == 129600);
        UMP_CHECK(ParseDuration("2h30m15s", 1000000, s) && s == 9015);
        UMP_CHECK(!ParseDuration("", 1000000, s));
        UMP_CHECK(!ParseDuration("0", 1000000, s));
        UMP_CHECK(!ParseDuration("-5m", 1000000, s));
        UMP_CHECK(!ParseDuration("5x", 1000000, s));
        UMP_CHECK(!ParseDuration("h", 1000000, s));
        UMP_CHECK(!ParseDuration("12d", 86400, s));
        UMP_CHECK(!ParseDuration("99999999999999999999", 1000000, s));
    }
}

void RunTests()
{
    WallTimeByDefault();
    SpeedReanchorsWithoutSkipping();
    JumpAddsOnTopOfSpeed();
    Durations();
}
