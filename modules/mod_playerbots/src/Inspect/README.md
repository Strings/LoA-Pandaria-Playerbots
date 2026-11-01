# Bot inspection

`.bot` commands that explain what a bot is doing and why (T-039; DESIGN.md, "GM tools (decided)", Inspect bots).
Permission 91004 (GM tools block), linked to role 197. Select the bot or give its name.

| Command | Shows |
|---|---|
| `.bot why [name]` | The last decision: triggers that fired, every queued candidate with its score and the trigger that pushed it, what was evaluated in order with its result, and what ran. The first call turns tracing on for that bot. |
| `.bot why off [name]` | Turns tracing off. |
| `.bot state [name]` | Level, alive, engine state, and each engine's last action and strategies. |
| `.bot traits / memory / skill [name]` | The section a bot system registered. Until a system owns it, it says which system will. |
| `.bot explain <section> [name]` | Any registered section. |
| `.bot sections` | Registered and planned sections. |

## Decision trace

`DecisionTrace.h` is plain data filled by `Engine::DoNextAction` when tracing is on for that engine. A traced tick
records the triggers, the queue as evaluation starts, each evaluated action's result (OK, FAILED, IMPOSSIBLE,
USELESS, PREREQ, ZEROED, UNKNOWN) and the chosen action. A tick that ran an action becomes the last decision; idle
ticks are kept separately. Untraced bots pay one pointer check per tick.

Scores are the engine's relevance values: the queued value, then after multipliers. Utility scores from later
systems (traits, mood, social coupling) should show up here as multipliers or relevance, so `.bot why` keeps
explaining them.

## Registering a section

```cpp
#include "BotInspect.h"

BotInspect::Register("traits", "Personality traits", [](Player* bot, std::vector<std::string>& lines)
{
    lines.push_back("boldness 0.7, sociability 0.3");
});
```

Register once at startup. `BotInspect::LastDecisionSummary(bot)` gives a one-line answer for tools such as capture
tickets: the traced choice when tracing is on, else the engine's last action.
