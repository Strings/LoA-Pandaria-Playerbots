/*
 * Pure math for CustomClock (T-039), kept free of server headers so the unit tests can build it alone.
 */

#ifndef __CUSTOMCLOCKRULES_H
#define __CUSTOMCLOCKRULES_H

#include <cstdint>

namespace CustomClock
{
    // A piecewise-linear clock: it runs at `Speed` times real time since the last anchor. Changing the speed or
    // jumping re-anchors at the current reading, so the clock never moves backwards and never skips on a change.
    struct Model
    {
        int64_t AnchorRealMs = 0;
        int64_t AnchorClockMs = 0;
        double Speed = 1.0;

        int64_t NowMs(int64_t realMs) const
        {
            return AnchorClockMs + static_cast<int64_t>(static_cast<double>(realMs - AnchorRealMs) * Speed);
        }

        void Anchor(int64_t realMs)
        {
            AnchorClockMs = NowMs(realMs);
            AnchorRealMs = realMs;
        }

        void SetSpeed(int64_t realMs, double speed)
        {
            Anchor(realMs);
            Speed = speed;
        }

        void Jump(int64_t realMs, int64_t forwardMs)
        {
            Anchor(realMs);
            AnchorClockMs += forwardMs;
        }

        void Reset(int64_t realMs)
        {
            AnchorRealMs = realMs;
            AnchorClockMs = realMs;
            Speed = 1.0;
        }

        // How far the clock is ahead of real time.
        int64_t AheadMs(int64_t realMs) const { return NowMs(realMs) - realMs; }
    };

    // Parses a duration like "90", "90s", "15m", "6h", "2d" or "1d12h" into seconds. False on anything else,
    // on zero, or on overflow past maxSeconds.
    inline bool ParseDuration(char const* text, int64_t maxSeconds, int64_t& seconds)
    {
        if (!text || !*text)
            return false;

        int64_t total = 0;
        while (*text)
        {
            if (*text < '0' || *text > '9')
                return false;

            int64_t value = 0;
            while (*text >= '0' && *text <= '9')
            {
                value = value * 10 + (*text - '0');
                if (value > maxSeconds)
                    return false;
                ++text;
            }

            int64_t unit = 1;
            switch (*text)
            {
                case 's': unit = 1; ++text; break;
                case 'm': unit = 60; ++text; break;
                case 'h': unit = 3600; ++text; break;
                case 'd': unit = 86400; ++text; break;
                case '\0': break;
                default: return false;
            }

            if (value > (maxSeconds - total) / unit)
                return false;
            total += value * unit;
        }

        if (total <= 0)
            return false;
        seconds = total;
        return true;
    }
}

#endif
