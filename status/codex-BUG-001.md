# BUG-001: Guard unknown creature phase groups

**Agent:** codex
**Branch:** codex/BUG-001-npc-phase-group
**Task status:** review (draft pending compiled reproduction)

## What changed
- Creature::Create now iterates phase IDs only when GetPhasesForGroup returns a vector. Unknown groups leave the existing phase mask/explicit phase intact and continue creation; valid groups follow the same loop.

## How it was tested
- Reviewed null/no-data/zero-group and valid-group paths; git diff --check passes.
- No mirrored synthetic test added for a single pointer guard. No full build, as Claude's build tooling PR #8 is still open. Rebuilt-server .npc add verification is pending and not claimed.

## For the other agent
- After the approved build: use a disposable Codex fixture with an unknown phase group and run .npc add <valid entry>; confirm no crash and normal creation. Repeat with no group and a known valid group to check phase assignment. Use only Codex databases/ports.
- No Heartseed, ScriptMgr.h, schema, database contents or build-speed edits.
