/*
 * CustomClock (T-039): the clock custom systems read for the durations they own (war front cycles, director
 * cooldowns, custom respawns and resets). It equals wall time until a GM speeds it up or jumps it with `.clock`,
 * so those cycles can be tested without waiting. Core systems keep reading time(nullptr): shifting the core's
 * clock would move some timers and not others. Not persisted: a restart returns it to wall time.
 */

#ifndef __CUSTOMCLOCK_H
#define __CUSTOMCLOCK_H

#include "Define.h"

#include <ctime>

namespace CustomClock
{
    // Current custom time (unix seconds / milliseconds).
    TC_GAME_API time_t Now();
    TC_GAME_API int64 NowMs();

    TC_GAME_API double GetSpeed();
    // Seconds the custom clock is ahead of wall time.
    TC_GAME_API int64 GetAheadSeconds();
    TC_GAME_API bool IsShifted();

    // speed > 0. Callers enforce the configured maximum.
    TC_GAME_API void SetSpeed(double speed);
    // Forward only: the clock never runs backwards.
    TC_GAME_API void Jump(int64 seconds);
    TC_GAME_API void Reset();
}

#endif
