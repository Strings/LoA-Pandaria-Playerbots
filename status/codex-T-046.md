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
- Pure eligibility policy; narrow lock/join/return hooks; native access/entrance and scaling integration; GM force/inspect; custom saved return anchor.
- SQL copy-of-real-save/idempotence checks, default normal-queue build, native round-trip/config/relog checks and full current regressions.
- T-041 verification remains first as soon as its build is free; T-044 build/live checks precede this branch's build.

## Map additions
- src/server/scripts/Custom/LegacyDungeons/ (planned adapter/policy).
