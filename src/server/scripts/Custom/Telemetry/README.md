# Player telemetry (T-004)

Logs how human characters play, so human-like bots can be trained offline (`docs/DESIGN.md`, "Human-like
bots from player telemetry" and "Decision context"). Off by default (`Telemetry.Enable`); logs stay on the
machine that runs the world server.

## Layout

| File | What |
|---|---|
| `TelemetryCore.h/.cpp` | Game-agnostic core: JSON rows, pause reasons, the discard window, activity episodes, movement sampling, pack geometry, fight tracking. No server headers; unit-tested in `tests/telemetry/`. Ships with the open bot brain. |
| `TelemetryMgr.h/.cpp` | WoW adapter: one session per tracked character, context snapshots from server state, file output. |
| `telemetry_scripts.cpp` | Script hooks and the `.telemetry` commands. |
| `src/server/game/Scripting/TelemetryHooks.h` | Core call sites ScriptMgr has no hook for (cast start, cast cancel, commands, loot, battleground and boss objectives), set once by this module. |

Only stock class play is captured: spells, cooldowns, movement, targets, resources. Custom systems
(Heartseed Gifts and the rest) add no fields; their abilities appear as ordinary casts of their spell IDs.

## Files

One JSON object per line, one file per session: `<Telemetry.Directory>/<YYYY-MM-DD>/<guid>-<login ms>.jsonl`
(UTC date; default directory `<repo>/telemetry/`, gitignored). Rows wait in memory for
`Telemetry.DiscardWindowMinutes` before they are written, so `.telemetry discard` can still drop them;
`session_start` is written at once.

Every row carries:

| Field | Meaning |
|---|---|
| `v` | Schema version (1). Added fields keep the version; changed or removed ones bump it. |
| `t` | Row type (below). |
| `ts` | Server time, Unix milliseconds. |
| `sid`, `pid` | Session ID and character GUID counter (stable IDs, never names). |
| `actor` | `human`, or `scripted_bot` for a bot tracked with `.telemetry track on`. |
| `ep` | Activity episode ID within the session. |
| `intent` | The current `.telemetry tag` intent, when one is set. |

### Row types

**Decision events** carry `ctx`, the context snapshot (below): `target`, `cast_start` (`spell`, `item`,
`cast_ms`, `channel`, `gaze_ms`: time since the character last turned), `cast_cancel` (`spell`, `pct`
through the cast, `channel`, `tgt_int_ready`: the target's interrupt was ready, so a fake cast), `combat_start`
(`pull`: the character opened the fight), `mount`, `dismount`, `taxi_start`, `release` (`since_death_ms`),
`loot_open` (`src`, `entry`, `dist`), `loot_roll` (`item`, `choice`), `emote` (`emote`), `chat` (`type`,
`lang`, `len`, `msg`; only with `Telemetry.Chat`).

**Outcome events:** `cast` (a cast completed; triggered spells are skipped), `kill` (`kind`, `entry` or
`class`, `lvl_gap`), `death` (`killer`), `fight_end` (`dur_ms`, `taken`, `done`, `kills`, `result`:
`killed`, `died` or `left`), `wipe` (`grp`), `resurrect` (`ghost`: released first, `dead_ms`).

**Movement:** `move` (`map`, `x`, `y`, `z`, `o`, raw movement flags `f` and `f2`, `spd`). Sampled at most
every `Telemetry.MovementSampleMs`; a movement flag change logs at once; standing still with the same facing
logs nothing. Facing-only changes are kept (gaze lead). Flight paths log only `taxi_start` and `taxi_end`.

**Objectives:** `objective` with `kind` `world_state` (battleground world state `id` and `val`) or `boss`
(encounter `id` and state `val`), plus the character's position. Distances to objectives are computed offline.

**Session and gaps:** `session_start` (`aid`, `class`, `race`, `lvl`, `build`), `session_end` (`dur_ms`),
`pause` (`reasons`), `resume` (`paused_ms`), `discard` (`span_ms`, `rows`), `episode` (`act`, `ctx`,
`since`), `intent` (`value`), `zone` (`map`, `zone`, `area`).

### Context snapshot (`ctx`)

| Part | Fields |
|---|---|
| `self` | `class`, `spec`, `lvl`, `hp` %, `pw` ([power type, %] for every power the character has), `buffs` and `debuffs` (spell IDs), `cd` ([spell, ms left] for spells cooling down), `ilvl`, `mounted`, `swim`, `fly`, `combat`, `spd`, `attackers`, `since_combat_ms`, `since_turn_ms`, `dr` (diminishing-returns group: level) |
| `tgt` | `kind`, `entry` or `class` and `spec`, `bot`, `rank`, `ctype`, `self`, `lvl_gap`, `hp`, `dead`, `hostile`, `dist`, `ang` (radians, 0 ahead, positive left), `dz`, `los`, `cast` and `cast_pct`, `victim` (`none`, `self`, `ally`, `other`), `int_cd` (player targets: interrupt cooldown in ms, null if none known), `dr`; null without a target |
| `env` | Attackable units within 5, 15 and 40 yards (`h5`, `h15`, `h40`) and their level gaps nearest first (`gaps`), `low` (below 20% health), friendly humans and bots within 40 yards (`fp`, `fb`), `grp` size, `roles` (tank, healer, damage), `pack` (`n`, `dist`, `ang`, `head`: the allies' centroid and mean heading relative to the character, within `Telemetry.PackRadius`), `on_tgt` (allies already attacking the target) |
| `place` | `map`, `zone`, `area`, `inst` (`world`, `dungeon`, `raid`, `bg`, `arena`, `scenario`), `indoors`, `sess_min` |

### Episodes

`act` is one of `fight`, `gathering` (a herb, ore, chest or skinning cast in the last 5 s), `vendor`
(targeting a vendor, bank, trainer, mailbox and similar within 8 yards), `idle` (no input for
`Telemetry.IdleEpisodeSeconds`), `travel` (mounted or on a flight path) and `questing` (anything else out in
the world). `ctx` is the map type. A new activity must hold for `Telemetry.EpisodeHoldMs`; fights, map type
changes and logging gaps start a new episode at once.

### Pauses

Logging pauses while GM mode is on, while AFK, after a GM command (accounts above player level; the
`.telemetry` commands are exempt) until the next move or cast, after `Telemetry.IdleTimeoutSeconds` without
input, and by hand. Paused stretches log only the `pause` and `resume` edges.

## Commands

| Command | Who | Does |
|---|---|---|
| `.telemetry` / `.telemetry status` | Player | Session, pause state, episode, intent, rows buffered and written |
| `.telemetry pause` / `resume` | Player | Manual pause; resume also ends GM-command and idle pauses |
| `.telemetry discard <minutes>` | Player | Drops the last stretch, up to the discard window |
| `.telemetry tag [intent]` | Player | Stamps an intent (`hunt-healer`, `spin-flag`) on every row until cleared with no intent |
| `.telemetry snapshot` | GM | Prints the context snapshot for the selected player or yourself |
| `.telemetry flush` | GM | Writes every buffered row now (they can no longer be discarded) |
| `.telemetry track on\|off` | GM | Logs the selected playerbot as `scripted_bot` (sparring, the bot-tells discriminator) |

Permissions: 95000 (player commands, role 199) and 95001 (GM commands, role 197), from
`sql/updates/master/auth/2026_11_02_claude_05_telemetry_commands.sql`.

## Config

All keys are in `worldserver.conf.dist` under PLAYER TELEMETRY: `Telemetry.Enable`, `Directory`, `Chat`,
`MovementSampleMs`, `IdleTimeoutSeconds`, `IdleEpisodeSeconds`, `EpisodeHoldMs`, `DiscardWindowMinutes`,
`FlushIntervalMs`, `PackRadius`, `InterruptSpells`. The test profile (`tools/devenv/profile.d/004-telemetry.json`)
turns logging on.

## Not yet

- The client addon for signals the server cannot see (UI panels opened) is a separate task.
- `flee` is not its own event: `fight_end` with `result` `left` plus the movement rows cover it offline.
