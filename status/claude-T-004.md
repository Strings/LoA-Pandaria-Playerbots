# T-004: Player telemetry logging hook

**Agent:** claude
**Branch:** claude/T-004-telemetry
**Task status:** done

## What changed
- Server-side telemetry for human characters, off by default (`Telemetry.Enable`). Rows are JSON lines, one file per session, under `<repo>/telemetry/` (gitignored). They cover movement (raw flags, facing changes kept), casts and cancels, target changes, combat starts and pulls, mounts, flight paths, releases, loot opens and rolls, emotes, chat (its own key), kills, deaths, fight summaries, wipes, resurrections, battleground and boss objectives, activity episodes, intents and pause gaps. The schema is versioned, every row carries `actor`, and only stable IDs are logged.
- Decision events carry the full context snapshot from DESIGN.md "Decision context":
  - Self: resources, auras, cooldowns, item level, movement state, time since combat and since the last turn, diminishing returns.
  - Target: identity, level gap, rank, distance, angle, height, line of sight, cast and progress, its victim, a player target's interrupt cooldown, diminishing returns.
  - Surroundings: hostiles at 5, 15 and 40 yards with level gaps, enemies near death, friendly humans and bots, group roles, the pack's centroid and heading, allies already on the target.
  - Place and time.
- Commands:
  - Players: `.telemetry status|pause|resume|discard <min>|tag [intent]`.
  - GMs: `snapshot`, `flush`, and `track on|off`, which logs a playerbot as `scripted_bot`.
- Automatic pauses: GM mode, AFK, GM commands (until the next move or cast) and idle.
- Game-agnostic core (`TelemetryCore`, no server headers) plus a WoW adapter, in `src/server/scripts/Custom/Telemetry/`. Core call sites go through `src/server/game/Scripting/TelemetryHooks.h` (no `ScriptMgr.h` edit): `Spell::prepare`, cancel cast and channel, `ChatHandler::ParseCommands`, loot open and roll, `Battleground::UpdateWorldState`, `InstanceScript::SetBossState`.
- ID block 95000 for telemetry in `docs/ID_RANGES.md`; RBAC 95000 (player) and 95001 (GM).

## How it was tested
- `unit_telemetry_core`: JSON, pauses, the discard window, episodes, movement sampling, geometry, fights. Unit tests 21/21.
- New regression scenario `telemetry`: revive, walk, self-target, every command and its refusals. The written file was then checked: every line parses, and the `target` row has `self`, `tgt`, `env` and `place`.
- Full regression suite run before the PR.

## Decisions made
- **Storage:** JSON lines files, not database tables. They are append-only, cheap and easy for the offline Python side. "Logs stay local" is satisfied by the gitignored directory.
- **Discard:** rows wait in memory for `Telemetry.DiscardWindowMinutes` (15) before they are written, so a discard really deletes them. A crash loses at most that window. `session_start` is written at once.
- **Episodes:** `act` (fight, gathering, vendor, idle, travel, questing) plus `ctx` (the map type), rather than mixing dungeon and PvP into the activity list. `questing` is the catch-all for active play out in the world.
- **GM-command pause:** applies only to accounts above player level; `.telemetry` itself is exempt.
- **Interrupt spells:** a config list (`Telemetry.InterruptSpells`), so it can be fixed without a rebuild.
- **Objectives:** battleground world-state changes and boss state changes. Flag carriers show up in world states and auras; no per-map code.
- **Module location:** the logic lives under `src/server/scripts/Custom/Telemetry/`, not the `src/server/game/Telemetry/` named in the task, so `game/CMakeLists.txt` stays untouched. Only the hook dispatch is in `game/Scripting`.

## For the other agent
- Apply `sql/updates/master/auth/2026_11_02_claude_05_telemetry_commands.sql` to your auth database for the `.telemetry` permissions.
- The test profile turns telemetry on (`tools/devenv/profile.d/004-telemetry.json`). Files land in your worktree's `telemetry/` folder (gitignored).
- New core call sites, guarded by a null check: `Spell.cpp` (prepare), `SpellHandler.cpp`, `Chat.cpp`, `LootHandler.cpp`, `GroupHandler.cpp`, `Battleground.cpp`, `InstanceScript.cpp`.

## Next
- Telemetry addon for client-only signals (UI panels opened).
- T-059 sparring can use `.telemetry track on` and `actor` for the bot side.

## Map additions
- `src/server/scripts/Custom/Telemetry/`: player telemetry (core, adapter, commands, schema README).
- `src/server/game/Scripting/TelemetryHooks.h`: core call sites for telemetry.
