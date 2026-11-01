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
- SQL validator: both files pass, zero issues. Applied both twice to disposable world_codex_t041_5d7991ef populated with actual source NPC/auth rows: identical second result and every original row unchanged. Expected two templates/spawns and text/permission present. Report: ignored build/T041-sql-check.json. Codex runtime SQL has NOT been applied (T-014 must not see unregistered scripts).
- Production C++ has NOT been built or live-tested yet. Primary checkout is held on T-014 while its build waits for the shared build mutex.

## Next / resume
- Finish T-014 build/live checks first. Do not switch primary while its queued build is active.
- Detach this secondary checkout, switch primary to this branch, and reconfigure the default build. Commit/push before building.
- Apply world 2026_11_01_codex_02_solo_qol.sql and auth 2026_11_01_codex_03_solo_qol_permissions.sql twice to Codex DBs; verify repeatability, no other rows changed.
- Live: .soloqol status/perks/hearth/hearthreset/service; real cast and NPC gossip; guild leave/reload/config off; neutral/bot service guard; cloth-on-plate transmog; all three headless regressions. Existing fixtures and private ports are described in tools/devenv; credentials stay ignored.
- No core-game file changes. No main SQL until approved/merged. T-013/T-011 reviews are approved without required fixes and held for human merge.
