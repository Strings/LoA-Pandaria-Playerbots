# T-081: Spawn thinning at load

**Agent:** claude
**Branch:** claude/T-081-spawn-thinning
**Task status:** done

## What changed
- New module `src/server/scripts/Custom/SpawnThinning/`. At startup it leaves hostile open-world spawns closer than `SpawnThinning.Radius` to an older spawn out of the map grids, so pulls stop bringing three or more mobs.
- World data is untouched: the spawns stay in the DB and in `sObjectMgr`. Setting `Radius = 0` and restarting brings them all back.
- Never thinned:
  - scripted spawns (by spawn or template), critters, NPCs with any npcflag, rares and bosses
  - spawns used by formations, linked respawns, per-spawn SmartAI, waypoint paths, pools or game events
- Config keys: `SpawnThinning.Radius` (conf.dist 10; 0 in code when the key is missing), `SpawnThinning.SameEntryOnly` (0), `SpawnThinning.Maps` ("0 1 530 571 870").
- GM commands `.spawnthin` (summary) and `.spawnthin near [yards]`, RBAC 91009 (`sql/updates/master/auth/2026_11_01_claude_08_spawnthin_command.sql`).

## How it was tested
- Unit tests: `tests/spawn_thinning` (7 cases: off, oldest stays, spacing, thinned spawns don't block, cell borders, groups, negative coordinates).
- Devenv log: `>> Spawn thinning: left 18495 of 85544 hostile open-world spawns out (radius 10.0 yd) in 280 ms`.
- New scenario `spawn-thinning`: `.spawnthin`, then a teleport to the Riverpaw camp in Westfall and `.spawnthin near 100`, which lists 12 thinned spawns. heartseed-gifts, heartseed-sync and heartseed-xuen also pass.
- The Heartseed scenarios now start with `.revive`, because a dead Seedtest left over from an earlier run failed every cast.

## Decisions made
- Thinning is cross-type by default (one camp per 10 yd, whatever the mob), not just same-entry, because mixed camps were what pulled 3+.
- Greedy by spawn id, so the oldest spawn of a crowd stays.

## For the other agent
- Hotspots: ScriptLoader (`AddSC_spawn_thinning`), worldserver.conf.dist (SPAWN THINNING section at the end).
- Apply the auth SQL to your auth DB if you want `.spawnthin` on a non-admin account.
- The test account lacks `go xyz` permission, so `.go xyz x y z map` falls through to the bare `.go` handler with "xyz" read as X. Use `.go x y z map` in scenarios.

## Next
- Tune the radius from playtest feedback. Respawn rates may need to rise slightly in thinned zones.

## Map additions
- `src/server/scripts/Custom/SpawnThinning/` (already added).
