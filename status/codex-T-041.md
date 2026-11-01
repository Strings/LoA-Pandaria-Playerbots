# T-041: Script quality-of-life changes

**Agent:** codex
**Branch:** codex/T-041-solo-qol
**Task status:** in progress; draft PR 85

## What changed
- Script half only. Flash's stack data is excluded. Based on playerbot a273234b.
- Guild perks reconcile on login/reload and after native join/leave work, using a dirty-player queue rather than a timer. Disabling restores native membership/level rules.
- Existing mixed armor/weapon transmog settings already ship enabled; status reads the actual loaded transmog module values. No duplicate core implementation.
- Free services were prototyped in Lua with two Lua VM tests, then ported to a guarded C++ gossip script. Two neutral city stewards offer rename/customize/race/faction flags. Bots, neutral Pandaren and combat are excluded. Unrelated at-login flags are preserved.
- Hearthstone AfterCast replaces the existing spell/category cooldown through public APIs. Every cast reads the configurable final duration; zero clears it, negative retains native behavior. GM inspect/force/reset/service commands use permission 94000.
- Reserved SoloQoL IDs and added scoped, idempotent world/auth SQL. NPC respawn delay and Hearthstone duration are config keys.

## Verification so far
- All 15 unit suites pass, including new pure policy boundaries; both Lua VM prototype tests pass.
- SQL validator: both files pass, zero issues. Applied both twice to disposable world_codex_t041_5d7991ef populated with actual source NPC/auth rows: identical second result and every original row unchanged. Expected two templates/spawns and text/permission present. Report: ignored build/T041-sql-check.json. World/auth SQL is now applied to Codex runtime DBs only; its worldserver is stopped until the matching binary is built. Main DBs unchanged.
- Production C++ build is queued in the primary checkout (build/T041-build.log); live checks remain. T-014 completed build/live/regressions and PR 82 is ready, including the approved rebased push.

## Next / resume
- Wait for the current default build; do not change primary C++ inputs while queued/running. Secondary Codex checkout is on T-014 for its completed post-push recheck.
- World/auth runtime SQL already applied; repeatability/preservation were checked in the disposable copy. Private profile uses Hearthstone 60 seconds, services respawn 2 seconds, all features/intro on; restore 300/300 defaults after checks.
- Native headless item-hotfix/transmog packet helper reused from T-015; neutral Pandaren language fix reused from T-014. Test Hearthstone item GUID 8 in character 1 bag 0 slot 23. Fixture 13 is human, 14 neutral Pandaren; both at_login were zero before services testing.
- Live: .soloqol status/perks/hearth/hearthreset/service; real cast and NPC gossip; guild leave/reload/config off; neutral/bot service guard; cloth-on-plate transmog; all three headless regressions. Existing fixtures and private ports are described in tools/devenv; credentials stay ignored.
- No core-game file changes. No main SQL until approved/merged. T-013/T-011 reviews are approved without required fixes and held for human merge.
