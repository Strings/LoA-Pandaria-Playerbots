# T-011: First-login starting kit

**Agent:** codex
**Branch:** codex/T-011-starting-kit
**Task status:** review

## What changed
- Configurable first human login grants Apprentice Riding (75), a racial ground mount, and Heavy Brown Bags (4497, ten slots) in empty bag slots 19-22. Both keys default on.
- Neutral/allied Pandaren use Green Dragon Turtle (120395). Existing bags and higher riding are preserved.
- Small startup-only FirstLoginHooks callback dispatches before core clears AT_LOGIN_FIRST. ScriptMgr interface and legacy unregistered PlayerStartScript remain unchanged.
- Bot WorldSession classification skips the kit before BotFactory initialization. GM startingkit inspects selected player/self.

## How it was tested
- Rebased onto playerbot 5f3b23a7. Default RelWithDebInfo worldserver build passes with no new warnings in touched files. All 12 current unit suites pass, including race mapping and first-login callback registration/order. All current regression scenarios pass: capture-idea, custom-reload-rng and login.
- Live: human gets riding 75, racial mount and four 10-slot bags. Neutral race-24 Pandaren learns and successfully mounts Green Dragon Turtle with GM mode off. Mount-only, bags-only and both-off configurations pass; enabling after first login does not backfill. Relog preserves the same four bag GUIDs.
- Playerbot first login skips the kit before its own AI/gear initialization; normal bot init equips four Portable Holes, with zero kit bags and identical bag GUIDs after relog. No random-bot load used.
- Final current-base binary: newly created human Kitfinal and neutral Pandaren Kitpandaf receive the correct kit and preserve all four bag GUIDs after relog. Low-level staging runs use GM protection to allow clean logout in Horde territory.
- Headless neutral Pandaren language mapping is corrected to LANG_PANDAREN_NEUTRAL (42), necessary for its live command checks. Full SOAP output for regression assertions and an empty-bot-safe Capture baseline fix existing harness false negatives; Capture unit tests retain actual last-decision serialization coverage.

## Decisions made
- First login owns the durable core flag; no played-time heuristic, backfill, SQL, new timers or dependencies.

## For the other agent
- Core changes require human review; do not merge. No Heartseed source or shared runtime changes.

## Next
- Human review: this PR touches the core first-login call site. Do not merge.

## Map additions
- Scripting/FirstLoginHooks, Custom/StartingKit and tests/starting_kit.

## Review follow-up
- Claude approved with no required logic changes. Rebased on playerbot 2c654e81 as requested; kept both append-only loader/config additions and reused the already-merged neutral Pandaren harness fix. Starting-kit module/tests are unchanged; existing policy/callback binary rerun passes. Build/native results above precede this rebase; human core merge remains held. No SQL.
