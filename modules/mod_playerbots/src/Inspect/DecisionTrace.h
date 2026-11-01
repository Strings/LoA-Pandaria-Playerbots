/*
 * Decision trace for `.bot why` (T-039): what one Engine tick considered and what it ran. Off by default; a GM
 * turns it on per bot, so untraced bots pay one null-pointer check per tick. Plain data with no server includes,
 * so later inspection tools (captures, telemetry) can read it without the engine.
 */

#ifndef _PLAYERBOT_DECISIONTRACE_H
#define _PLAYERBOT_DECISIONTRACE_H

#include <cstdint>
#include <ctime>
#include <string>
#include <vector>

struct DecisionCandidate
{
    std::string Name;
    float Relevance = 0.0f;  // as queued, before multipliers
    std::string Source;      // the trigger that pushed it, or empty for strategy defaults and follow-ups
};

struct DecisionStep
{
    std::string Name;
    float Relevance = 0.0f;  // after multipliers
    char const* Result = ""; // OK, FAILED, IMPOSSIBLE, USELESS, PREREQ, ZEROED, UNKNOWN
};

struct BotDecision
{
    time_t Time = 0;
    std::string EngineState;                // combat, non-combat, dead
    std::vector<std::string> Triggers;      // triggers that fired this tick
    std::vector<DecisionCandidate> Queue;   // the queue when evaluation started, highest first
    std::vector<DecisionStep> Steps;        // what was evaluated, in order
    std::string Chosen;                     // empty when nothing ran
    float ChosenRelevance = 0.0f;
};

class DecisionTrace
{
public:
    void Begin(time_t now, std::string engineState)
    {
        _working = BotDecision();
        _working.Time = now;
        _working.EngineState = std::move(engineState);
    }

    BotDecision& Working() { return _working; }

    // A tick that ran an action becomes the last decision; idle ticks are kept separately so `.bot why` can show
    // both "what it last did" and "what it is considering now".
    void End()
    {
        if (!_working.Chosen.empty())
        {
            _last = _working;
            _hasLast = true;
        }
        else
            _lastIdle = _working;
        ++_ticks;
    }

    bool HasLast() const { return _hasLast; }
    BotDecision const& Last() const { return _last; }
    BotDecision const& LastIdle() const { return _lastIdle; }
    uint64_t Ticks() const { return _ticks; }

private:
    BotDecision _working;
    BotDecision _last;
    BotDecision _lastIdle;
    bool _hasLast = false;
    uint64_t _ticks = 0;
};

#endif
