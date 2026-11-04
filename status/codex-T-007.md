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
