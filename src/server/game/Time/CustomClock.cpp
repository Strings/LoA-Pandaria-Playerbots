/*
 * CustomClock (T-039). See CustomClock.h.
 */

#include "CustomClock.h"
#include "CustomClockRules.h"

#include <chrono>
#include <mutex>

namespace
{
    std::mutex ClockLock;

    int64_t RealMs()
    {
        return std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
    }

    CustomClock::Model& State()
    {
        static CustomClock::Model model = []
        {
            CustomClock::Model m;
            m.Reset(RealMs());
            return m;
        }();
        return model;
    }
}

namespace CustomClock
{
    int64 NowMs()
    {
        int64_t real = RealMs();
        std::lock_guard<std::mutex> guard(ClockLock);
        return State().NowMs(real);
    }

    time_t Now()
    {
        return time_t(NowMs() / 1000);
    }

    double GetSpeed()
    {
        std::lock_guard<std::mutex> guard(ClockLock);
        return State().Speed;
    }

    int64 GetAheadSeconds()
    {
        int64_t real = RealMs();
        std::lock_guard<std::mutex> guard(ClockLock);
        return State().AheadMs(real) / 1000;
    }

    bool IsShifted()
    {
        int64_t real = RealMs();
        std::lock_guard<std::mutex> guard(ClockLock);
        Model const& model = State();
        return model.Speed != 1.0 || model.AheadMs(real) >= 1000;
    }

    void SetSpeed(double speed)
    {
        if (!(speed > 0.0))
            return;

        int64_t real = RealMs();
        std::lock_guard<std::mutex> guard(ClockLock);
        State().SetSpeed(real, speed);
    }

    void Jump(int64 seconds)
    {
        if (seconds <= 0)
            return;

        int64_t real = RealMs();
        std::lock_guard<std::mutex> guard(ClockLock);
        State().Jump(real, seconds * 1000);
    }

    void Reset()
    {
        int64_t real = RealMs();
        std::lock_guard<std::mutex> guard(ClockLock);
        State().Reset(real);
    }
}
