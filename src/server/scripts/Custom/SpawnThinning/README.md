# Spawn thinning

The imported world data packs open-world mobs tightly (a Riverpaw gnoll camp in Elwynn had 53 gnolls about
11 yards apart, so every pull brought three or more). At startup this module leaves hostile spawns that crowd an
older one out of the map grids. The world database is not changed: the spawns stay in `creature` and in
`sObjectMgr`, and turning thinning off brings them all back after a restart.

## Files

| File | What |
|---|---|
| `SpawnThinningRules.h` | Pure rule: greedy in spawn id order, a spawn is thinned when a kept spawn of its group is closer than the radius. Game-agnostic; unit tests in `tests/spawn_thinning/`. |
| `spawn_thinning_scripts.cpp` | `OnStartup` pass over the creature data (before any grid loads) and the `.spawnthin` command. |

## What is thinned

Only spawns on `SpawnThinning.Maps` whose template is hostile to players, normal or elite rank, not a critter, with
no NPC flags and no script name. Never thinned: spawns in formations, linked respawns, SmartAI by spawn guid,
waypoint patrols (`creature_addon.path_id`), pools and game events. Groups are map plus phase (mask, id, group),
plus the entry with `SpawnThinning.SameEntryOnly`.

## Config

`SpawnThinning.Radius` (yards, 0 = off; 10 ships), `SpawnThinning.SameEntryOnly`, `SpawnThinning.Maps`. Read at
startup only.

## GM commands

- `.spawnthin`: radius, maps and how many spawns were left out.
- `.spawnthin near [yards]`: thinned spawns around you (up to 10 listed, with spawn id, entry and distance).

Permission 91009 (`sql/updates/master/auth/2026_11_01_claude_08_spawnthin_command.sql`), role 197.
