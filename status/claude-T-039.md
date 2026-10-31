# T-039: GM tools, first set

**Agent:** claude
**Branch:** claude/T-039-gm-tools
**Task status:** review

## What changed
- **Clock:** `CustomClock` in `src/server/game/Time/` plus `.clock info|speed|jump|reset`. It is the clock custom systems
  read for their own cycles; clients' day and night follow it. Pure math in `CustomClockRules.h`.
- **Bot inspection** (`modules/mod_playerbots/src/Inspect/`):
  - `.bot why` shows a per-bot decision trace: triggers, queued candidates with scores and sources, evaluation order
    with results, and what ran.
  - `.bot state`, `.bot sections` and `.bot explain`.
  - `.bot traits|memory|skill` answer through a `BotInspect` section registry that later bot systems fill.
  - Small hooks in `Engine.cpp/.h`, `Queue.h` (read-only walk) and `PlayerbotAI.h` (`GetEngine`).
- **Scenario loader:** `.scenario list|explain|load` over the data tables `gmtools_scenario` and
  `gmtools_scenario_step`. Steps are GM command lines with placeholders; the tables reload with
  `.reload gmtools_scenario`. Three seed scenarios.
- **Spectate:** `.spectate <name>` / `.spectate stop`: GM invisible, camera on the bot, cleanup on logout and map change.
- **Config keys:** `GMTools.Clock.MaxSpeed`, `GMTools.Clock.MaxJumpSeconds`, `GMTools.Spectate.AttachTimeoutMs`.
- **SQL:** `sql/updates/master/auth/2026_10_31_claude_18_gmtools_t039.sql` (RBAC 91003 to 91006) and
  `sql/updates/world/2026_10_31_claude_19_gmtools_scenario.sql`. Applied by hand to `auth_claude` and `world_claude`,
  twice, to check they can be re-run.

## How it was tested
- Unit tests: `unit_custom_clock` and `unit_scenario_rules` are new; all 7 suites pass.
- Live checks on the Claude server: see the PR.

## Decisions made
- **The clock is a separate custom clock, not the core clock.** The core reads `time(nullptr)` in about 240 places
  and `GameTime` in about 30, so offsetting either would shift some core timers and not others. Custom systems read
  `CustomClock`; core respawns and resets keep their own GM commands. DESIGN.md's list (war fronts, director
  cooldowns, day and night) is covered; core respawns and raid resets go through the existing `.respawn` and
  `.instance` commands.
- **`.bot why` scores are the playerbots engine's relevance values,** since that is the bot's real utility today.
  Later utility systems should feed relevance or multipliers so the trace keeps explaining them.
- **The trace is per bot and opt-in** (the first `.bot why` turns it on), so the 300+ untraced bots pay one pointer
  check per tick.
- **Scenarios are command lists,** not a new step-type system like checkpoints, so any system's GM commands can be
  scripted with no code. A scenario cannot start another one.

## For the other agent
- **New custom timers should read `CustomClock::Now()`**, so `.clock` can test them.
- **Gemini (T-040):** use `BotInspect::LastDecisionSummary(bot)` (`modules/mod_playerbots/src/Inspect/BotInspect.h`)
  for each nearby bot's `last_decision`.
- **Engine.h changed,** so the whole playerbots module recompiles once after this merges.
- **New bot systems:** register their `.bot` section with `BotInspect::Register`.

## Next
- Later systems add their force commands, overlays and `.bot` sections.
- Possible follow-up: a GMSuite addon tab for `.clock`, `.scenario` and `.spectate`.

## Map additions
- `src/server/game/Time/CustomClock.h`: clock for custom systems' timers (T-039)
- `src/server/scripts/Custom/GMTools/README.md`: GM tools overview (clock, scenarios, spectate)
- `modules/mod_playerbots/src/Inspect/`: `.bot` inspection, decision trace, section registry
