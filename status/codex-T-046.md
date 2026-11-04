# T-046: Solo outleveled dungeon travel

**Agent:** codex
**Branch:** codex/T-046-legacy-dungeons
**Task status:** review (PR 91; core review and real-client list playtest pending)

## What changed
- Specific normal/heroic five-player Dungeon Finder joins become solo round-trip travel at the configured margin (default 20 levels over the dungeon cap), without matchmaking groups or completion rewards. Below-margin bot parties await the instance pool.
- Public `.legacy list/status/enter/out`; GM selected-player inspection and `force`, which bypasses only the level margin. Native entry restrictions and group/queue/combat/travel safety checks remain.
- A custom character table saves the return point and original difficulty before transfer. Logout/restart preserves the trip; returning, leaving by portal/hearth or deleting the character clears it. Existing trips can return after disabling new entries.
- Narrow core hooks cover native upper-level locks, joins and teleport-out. AutoBalance leaves active legacy visits at original difficulty and refreshes when visits change.
- Added configuration, player introduction, permissions, pure policy tests, documentation and a repeatable native entry/return regression scenario.
- Integrated playerbot 9a656fb4 on 2026-11-04, preserving published branch history. Telemetry keeps its 95000 block; legacy travel uses 96000. Both sides' configuration additions are retained.

## How it was tested
- Current production worldserver build PASS (RelWithDebInfo, normal shared build queue, 7.7 minutes). No warnings in touched files; existing PathGenerator and playerbots transport warnings remain outside this change. Log: `build/T046-current-build.log`.
- All 23 current standalone unit suites PASS, without warnings. Log: `build/T046-current-units.log`.
- Both new SQL files: validator 0 issues. Original strict migration check on a real characters_codex save copy preserved all 147 pre-existing tables, retained a representative trip and produced identical results across two applications. Report: `build/pr-validation/build/T046-migration-checks/characters_codex_check_83fe78972e4b/report.json`.
- Original implementation checks PASS: exact margin, GM force, live margin reload, return after disabling entry, battleground queue guard, unscaled visits with AutoBalance enabled, and saved anchor/difficulty across a worldserver restart. Earlier full suite: 11/11 PASS.
- Current native Dungeon Finder entry/return regression PASS: no group, map 389 entered, original location and heroic difficulty restored. Report: `build/regression-T046-current-native/20261104T224029Z-2d68b566/results.json`.
- Current ordinary-player native entry/return PASS separately, with no group and saved anchor cleared. Log: `build/T046-current-public-native.log`.
- All 28 current scenario assertions PASS across the full run and setup-corrected reruns; no assertion weakened. Combined evidence: `build/T046-current-regression-summary.json`. Six corrected reruns: `build/regression-T046-current-recheck/20261104T225227Z-55379450/results.json`; telemetry final: `build/regression-T046-current-telemetry/20261104T225702Z-4a50dd2b/results.json`.
- Temporary inputs adapt hardcoded Seedtest to Codextwink, use numeric self GUID 1, restore the expected initial Alchemy rank, and establish/sync telemetry's starting position. New command permissions required a private worldserver restart: `.reload rbac` refreshes account state but not global permission definitions.
- The initial telemetry timeout exposed a small test-runner bug mixing text/bytes in TimeoutExpired output. Fixed incidentally so timeouts produce a failed report instead of crashing. All 3 runner tests PASS, including text/bytes/mixed/empty timeout output and redaction; a real subsequent timeout also reported cleanly.

## Database/runtime notes
- Character 06 and auth 07 SQL applied manually to Codex databases only. No main SQL or deployment performed.
- Current private test/none runtime: `build/T046-current-runtime`, ports 3744/8105/7898. Updated only Codex's Heartseed data/schema and command permissions for current regressions; touched tables backed up under `build/T046-current-db-backup/`.
- Telemetry output stays inside this private runtime. New entries enabled; margin 20. No duration added.

## Decisions and remaining review
- Accepted interim cap: native DBC maximum, falling back to minimum if absent. Phase 6 replaces one lookup with the scaling cap table.
- Client dungeon-list visibility are unverified. Native protocol entry and return are verified; GUI testing still requires permission under AGENTS.md.
- Core hooks await Claude/human review on PR 91. Do not self-merge this core change.

## Map additions
- `src/server/scripts/Custom/LegacyDungeons/`: adapter, pure eligibility/ID parsing and README.
- `src/server/game/DungeonFinding/SoloDungeonHooks.h`, `.cpp`: optional adapter facade.
- `tests/legacy_dungeons/`: pure policy tests.
- `tests/regression/scenarios/legacy-dungeons.json`: native solo Dungeon Finder round trip.
