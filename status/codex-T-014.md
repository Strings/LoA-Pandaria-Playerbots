# T-014: All professions and extra profession bar

**Agent:** codex
**Branch:** codex/T-014-profession-bar
**Task status:** review (PR 82)

## What changed
- Existing MaxPrimaryTradeSkill ships as 11. The core fallback for an omitted key remains 2; no core-game changes.
- Own Professions script refreshes free slots on login/online config reload, clamps at zero without unlearning skills, and provides GM `.professions status` / `learnall` (existing add-item permission). Force learn preserves existing ranks and is repeatable.
- ProfessionBar addon shows known primary professions beyond the client's first two, scans all eight MoP spell ranks, uses secure localized spell buttons, and defers protected changes in combat. Gathering buttons use their actual gathering actions.
- Optional default-on login introduction. No new duration or SQL.
- Live test found the headless client used Common for neutral Pandaren; added their native language 42. This small test-helper fix lets their commands reach normal parsing.

## Verification
- Rebased/current on origin/playerbot da84b706 (base advance is documentation only). Default RelWithDebInfo worldserver build passes; no new warnings in touched files.
- All 15 CTest suites pass. Five Lua VM addon tests cover higher ranks/native-slot filtering, gathering actions, secure attributes/stale cleanup and combat deferral. No GUI client was launched.
- Native headless check on Codex ports/DBs, neutral Pandaren fixture 14: one existing profession at cap 2/free 1; online reload to 11/free 10; learnall twice gives eleven/free 0, persisted across relog; lowering cap to 2 retains all eleven/free 0; restored cap 11. Database count confirms eleven. Existing rank unchanged.
- All three regression scenarios pass: capture-idea, custom-reload-rng, login. Logs: ignored build/T014-*.log and build/regression/T014/20261101T162358Z-4176baad.

## Next / other agents
- Ready for Claude approval; routine merge only after approval and current-base check. Addon real-client visual layout was not tested; its secure behavior was exercised in Lua VM.
- No SQL/main DB application needed. Private runtime restored to cap 11.
