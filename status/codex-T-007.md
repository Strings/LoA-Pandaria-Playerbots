# T-007: Checkpoint fixtures and cold-file bot baseline

**Agent:** codex
**Branch:** codex/T-007-checkpoint-fixtures
**Task status:** in progress

## What changed
- Added test-only SQL templates at 1/30/60/85/90 using T-048 level, class_spells, gear, kit, riding and tele steps. No parallel character/item SQL implementation.
- Added private-runtime provisioning/verification, ownership guards and stable fixture GUID/equipment manifests. Dedicated GM test account, GM mode off on logout; no ordinary-player permission claims.
- Reuses T-037 for consistent cold-file snapshot/restore with paired auth/characters/world/playerbots and an existing 20..50-bot roster.

## Verification so far
- Five live checkpoint applications and equipment/level checks PASS on Codex-only databases. Nine guard/state tests PASS. Repeat/idempotence, bot initialization and physical baseline restore follow.

## For the other agent
- Main untouched. Fixture SQL lives under sql/fixtures intentionally; it is never a production update. Credentials remain in ignored runtime files.

## Next
- Verify repeat provisioning, initialize a small bot pool once, capture/restore and prove saved fixture/bot rows survive. Regression/checklist/PR update.

## Map additions
- tools/devenv/fixtures/: checkpoint fixture and baseline tooling.
- sql/fixtures/: test-only checkpoint templates and catalog.

## Baseline and preservation verification
- Repeat provision preserved the entire manifest and item GUIDs. Applying fixture SQL twice preserved its exact rows and every other checkpoint template; non-fixture character rows remained identical. Report: build/T007-live-report.json. Equipped row counts: 4/13/15/15/15 at levels 1/30/60/85/90.
- Added one-time test/bootstrap profile: three native bot accounts, automatic login/population zero. Then explicitly staged 20 bots through the existing local profile and initialized them at level 60. All 20 are alive/offline; 12 level-1 and one level-55 factory extras also remain offline. Ordinary test/none startup does not regenerate the pool.
- Snapshot build/fixture-snapshots/20261104T233259Z-9e4473e9 pairs all four Codex databases. Restored by the actual CLI to build/T007-restored/data; all 696 cold-file hashes matched. Started only that copied data on isolated port 3344, compared five fixture identities/levels and 62 item rows, 20 bot identities/levels and 314 item rows, 3889 bot spell rows, fixture account, and all playerbots table counts. All identical; isolated MySQL shut down cleanly. Report: build/T007-restore-report.json.
- All five authenticated world logins PASS via verify --login and their manifests stayed unchanged. Added wrong-character rejection coverage. Fixture GM mode is off on all five saved rows.
- Current 12 fixture tests and seven devenv tests PASS. Full regression run is in progress, with bots disabled and only temporary fixture identity/starting-position normalization (assertions unchanged).
