# BUG-001: Guard unknown creature phase groups

**Agent:** codex
**Branch:** codex/BUG-001-npc-phase-group
**Task status:** done

## What changed
- Creature::Create now iterates phase IDs only when GetPhasesForGroup returns a vector. Unknown groups leave the existing phase mask/explicit phase intact and continue creation; valid groups follow the same loop.

## How it was tested
- Reviewed null/no-data/zero-group and valid-group paths; git diff --check passes.
- No mirrored synthetic test added for a single pointer guard. No full build, as Claude's build tooling PR #8 is still open. Rebuilt-server .npc add verification is pending and not claimed.

## For the other agent
- After the approved build: use a disposable Codex fixture with an unknown phase group and run .npc add <valid entry>; confirm no crash and normal creation. Repeat with no group and a known valid group to check phase assignment. Use only Codex databases/ports.
- No Heartseed, ScriptMgr.h, schema, database contents or build-speed edits.

## Final validation (2026-10-31)
- Rebased onto playerbot with all task/status entries preserved. The exact Creature.cpp fix compiled in the full MSVC world/auth validation build, with no new warnings in Creature.cpp.
- Headless GM login: three `.npc add 721` commands created three expected rabbit rows. SOAP server info returned HTTP 200 afterward. Stopped the private world cleanly and removed exactly those three owned test rows.
- Unknown groups are guarded by the pointer check; the smoke spawns had group 0. No synthetic invalid DB group was inserted because other upstream spawn loaders still dereference unknown groups independently.
- Separate BUG-003: `.npc add 31144` works, but subsequent deletion of the added training dummies crashes in Aura::_UnapplyForTarget (spell 61574). This is not changed here; owned test rows were removed from the stopped private database.
