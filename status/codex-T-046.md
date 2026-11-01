# T-046: Solo outleveled dungeon travel

**Agent:** codex
**Branch:** codex/T-046-legacy-dungeons
**Task status:** in progress (independent preparation)

## What changed
- Started on latest playerbot aa0af9db after T-044 implementation/test preparation. Primary worktree remains frozen on T-041's queued build.
- Read decided dungeon-finder design and accepted interim state: solo only, original dungeon level until scaling table, instance-pool bot groups deferred.

## How it was tested
- All 15 standalone unit suites pass, including eligibility/cap fallback/boundary/force and strict decimal-ID parsing. Production C++ has not been built or live-tested.

## Decisions made
- Interim cap is the native DBC maximum level, falling back to its minimum when no maximum is supplied. This is replaced by one lookup when Phase 6 supplies caps.
- Plan a separate persistent return anchor so native battleground return data is not overloaded.

## For the other agent
- New AGENTS explicitly permits force-with-lease for one's own rebased branches; no new permission question is needed for that workflow.
- Core hooks will require human review/merge. No SQL applied yet.

## Implementation pushed (production verification pending)
- Adapter handles eligible native joins without forming a group or reward queue; below-margin bot parties remain deferred. Public `.legacy list/status/enter/out`, GM force/selected-player inspection.
- Saved return anchor and original difficulty survive logout/restart; return/portal/hearth arrival cleans the row. Character deletion cleans its state. Native access/lock restrictions remain; force bypasses the level margin only.
- Minimal lock/join/return hooks and an AutoBalance active-visit check preserve original difficulty. Existing trips can return after disabling new entries. No duration added.
- Reserved block 96000, character state and auth SQL. SQL/runtime/build/live checks remain pending.

## Next
- Policy, hooks, native access/scaling integration, GM controls and saved anchor are implemented; production compilation/live remain pending.
- SQL copy-of-real-save/idempotence checks, default normal-queue build, native round-trip/config/relog checks and full current regressions.
- T-041 verification remains first as soon as its build is free; T-044 build/live checks precede this branch's build.

## Map additions
- src/server/scripts/Custom/LegacyDungeons/ (planned adapter/policy).

## Migration verification
- Both SQL files pass lint, zero issues. Strict tools/backup/save_safety.py check against real characters_codex save passes: 147 pre-existing tables identical before/after, representative saved anchor retained, two applications produce identical schema/data. Report: ignored build/T046-migration-checks/characters_codex_check_83fe78972e4b/report.json. Scratch copy removed by the guard; original save read only. Canonical runtime/main SQL not applied yet.

- Rebased the prepared draft on playerbot 2c654e81 while T-044 waits in the normal queue. No main SQL; T-044 build/live still precede this branch. User approved guarded rebased publication for remaining Codex branches.

- First production compile found the core stores lock details in LfgLockStatusType rather than the enum directly; corrected comparisons to its lockStatus field. Rebuild follows through the normal queue.

- Production build now passes cleanly. All 16 current unit suites pass; fixed an unsigned test literal to remove MSVC optional-comparison warnings, and rechecking through the same shared mutex. Scoped character 06/auth 07 SQL is applied only to Codex runtime DBs. Matching private server is starting.

- Warning-free current units: all 16 pass. Native entry/return, saved original difficulty/anchor and unscaled visit PASS with AutoBalance explicitly enabled. Ordinary-player native DF entry/out creates no group and clears anchor. Exact threshold, GM force, live margin reload and return after disabling new entries PASS. Restart persistence and queue guard next.

- Saved visit and original return difficulty survive a full worldserver restart; native return clears the anchor afterward. Ordinary-player battleground queue blocks entry, queue cleaned. Original private AutoBalance.Global=0 restored; entry key/margin restored to 1/20. Full current regressions now running. Native client GUI/list filtering remains untested; backend native join/return packets and ordinary-player access are verified.

- All 11 current regressions PASS: build/regression-T046/20261101T204258Z-d69a35aa/results.json. Latest playerbot advances only Capture script/README; rebasing and rebuilding this base before ready, with legacy implementation unchanged.
