# T-007: Checkpoint fixtures and cold-file bot baseline

**Agent:** codex
**Branch:** codex/T-007-checkpoint-fixtures
**Task status:** review (PR 142)

## What changed
- Added five Orc warrior fixtures at levels 1/30/60/85/90. The test-only SQL presets reuse T-048 level, class_spells, gear, kit, riding and tele steps. No parallel gear implementation or raw character/item preset SQL.
- Private-runtime provision/verify commands enforce named agent worktrees, localhost, correct ports and all four database connections. Roster preflight rejects online/foreign characters. A dedicated CHECKPOINT_<AGENT> GM test account self-applies checkpoints and logs out with GM mode off; it is not a normal-permission test account.
- Stable fixture IDs map to actual character/item GUIDs in a private-runtime manifest. Repeating provision verifies recorded state without re-gearing. Explicit --reset reapplies checkpoint rules (upgrades only); exact resets use the saved baseline. verify --login authenticates each intended character and rechecks saved state.
- Added the test/bootstrap profile: three native bot accounts, zero population and automatic login. This creates the offline pool before a local roster exists. Local profile/stage_local then initializes a chosen 20..50-bot roster once; normal runs restore it.
- Snapshot/restore delegates to T-037, pairs auth/characters/world/playerbots, records the chosen bot roster and refuses existing restore destinations. No new schema, core hooks, IDs or production SQL. Operation limits are data in sql/fixtures/characters.json.
- GM-tools regression now accepts optional additional checkpoint templates while retaining its original minimum of 18.

## How it was tested
- Rebased on playerbot c54c886e. Production build PASS (build/T007-build.log and build/T007-latest-build.log); no C++ changes in this PR. Existing upstream PathGenerator/transport warnings remain.
- 23 C++ suites PASS: build/T007-final-units.log. 12 fixture guard/state tests and seven devenv tests PASS. Fixture SQL lint: 1 file, 0 issues.
- Five live checkpoint applications PASS, with 4/13/15/15/15 equipped rows. Repeated provision preserves the manifest and item GUIDs. SQL applied twice produces identical fixture rows and preserves all other checkpoint templates; non-fixture character rows remain identical. Report: build/T007-live-report.json.
- All five authenticated world logins PASS on the latest built binary, with saved state unchanged: build/T007-final-logins.log. Fixture GM mode is off on all five rows.
- Full clean run: all 28 current regressions PASS on the matching latest binary: build/regression-T007-final/20261105T000018Z-331af52d/results.json. Temporary inputs only normalize the orchestrator fixture's name, numeric self-target, initial profession rank and telemetry position/bookkeeping. Binary/source hashes: build/T007-binary-provenance.json.
- Initialized 20 bots once at level 60 in the local profile, then logged them out; all are alive/offline. Thirteen native factory extras remain offline. Snapshot: build/fixture-snapshots/20261104T233259Z-9e4473e9.
- Actual CLI file-copy restore to build/T007-restored/data matched all 696 cold-file hashes. Started only that copy on isolated MySQL port 3344; compared five fixture identities/levels and 62 equipped rows, 20 bot identities/levels and 314 equipped rows, 3889 bot spells, fixture account and all playerbots table counts. All identical; isolated MySQL shut down cleanly. Report: build/T007-restore-report.json.

## For the other agent
- Main runtime/databases untouched. Test-only SQL intentionally lives under sql/fixtures and must not be applied to main. No main SQL to deploy when merging this PR.
- Current Codex test/none runtime is build/T046-current-runtime (3744/8105/7898), bots disabled, custom clock reset. Bootstrap/staging runtimes are stopped. Preserve its ignored private credential/config files with any private backup; the public manifest contains no password.
- Restore requires the matching MySQL version (tested 8.0.36) and a new, worktree-local datadir on an unused local DB port. It never replaces the shared service.
- T-047 remains separately blocked by the human's client-crash report; crash fixes belong to Claude. No GUI or crash fix attempted.

## Map additions
- tools/devenv/fixtures/: checkpoint fixture provisioning, state/login verification and baseline wrapper.
- sql/fixtures/: test-only checkpoint templates, catalog and limits.
- tools/devenv/profiles.json: one-time offline bootstrap profile.
