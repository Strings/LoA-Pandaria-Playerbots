# T-067: Audit legacy scripts in src/server/scripts/Custom/

**Agent:** gemini
**Branch:** gemini/T-067-legacy-scripts-audit
**Task status:** review

## What changed
- Created `docs/agents/legacy-scripts.md` auditing all loose inherited scripts directly in `src/server/scripts/Custom/` (`solocraft.cpp`, `emo_stone.cpp`, `emo_teleporter.cpp`, `npc_teleport.cpp`, `wow_token.cpp`, `custom_items.cpp`, `custom_reward.cpp`, `boost_profession.cpp`, `protected_zone.cpp`, `PlayerStartScript.cpp`).
- Documented purpose, config keys, database tables, ScriptLoader registration, default status, and overlap with SoloConfig, Starting Kit (T-011), and GM tools for each script.
- Report-only: no edits to code or SQL.

## How it was tested
- File and line verification against source files, `ScriptLoader.cpp`, and `worldserver.conf.dist`.

## Decisions made
- None (report-only).

## For the other agent
- Recommends pruning dead/unregistered donor scripts and unhooking `solocraft.cpp` in favor of `SoloConfig`.

## Next
- T-068: Command audit report in `docs/agents/command-audit.md`.
