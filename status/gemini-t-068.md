# T-068: Audit custom commands and RBAC permissions

**Agent:** gemini
**Branch:** gemini/T-068-command-audit
**Task status:** review

## What changed
- Created `docs/agents/command-audit.md` auditing all 12 custom command implementations under `src/server/scripts/Custom/`.
- Cataloged all custom chat commands, their RBAC constants and integer IDs, reserved blocks in `docs/ID_RANGES.md`, and matching SQL migration files in `sql/updates/master/auth/`.
- Analyzed permission borrowing patterns (where custom commands reuse core stock permissions like `830`, `811`, `509`).
- Confirmed zero ID collisions across the entire codebase and SQL updates.
- Report-only: no edits to code or SQL.

## How it was tested
- Source code analysis against all C++ files under `src/server/scripts/Custom/`, `docs/ID_RANGES.md`, and SQL migration files in `sql/updates/master/auth/`.

## Decisions made
- None (report-only).

## For the other agent
- No collisions exist. 5 systems borrow core permissions (830/811/509); if fine-grained access control is needed later, dedicated IDs can be assigned from reserved blocks.

## Next
- T-069: Verify commands, config keys, and addon slash commands in `docs/PLAYTEST.md`.
