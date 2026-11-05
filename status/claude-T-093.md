# T-093: Hand-placed world treasures, first pass in Durotar

**Agent:** claude
**Branch:** claude/T-093-durotar-treasures
**Task status:** review

## What changed
- `src/server/scripts/Custom/Treasures/`: `.treasure validate <x> <y> <z> [map] [roadX roadY roadZ] [nudge]`, usable
  over SOAP (DESIGN.md "World placement tool", step 3). It loads the grids itself and reports snapped ground height,
  indoor/outdoor (vmap WMO ids), liquid, slope, five sky rays (Flying rule), line of sight, a navmesh walk from the
  road point with the same `PathGenerator` as `.mmap path` (legs chained, tried in both directions), and spacing.
  With `nudge` it searches rings around a failing spot for the best passing one. Pure rules in `TreasureRules.h`,
  unit-tested in `tests/treasures/`.
- Loot once per character: `character_treasure` (characters), `treasure_spot` (world, label and road point per spawn).
  Opening rolls the chest's loot for that character alone straight into the bags (curio, green from the spot's pool,
  8-15% Ragefire-era blue, small gold). The chest then stops sparkling and is hidden for that character only, through
  a small new core hook header `src/server/game/Scripting/GameObjectHooks.*` called from
  `GameObject::IsNeverVisibleFor` and the chest dynamic-flags update; others still see and open it.
- GM commands: `.treasure list | go | spawn | reset | looted | explain | validate | check | reload` (RBAC 99000).
- 12 Durotar treasures in `sql/updates/world/2026_11_05_claude_03_durotar_treasures.sql` (templates 909000-909011,
  reference pools 909100-909105, spawn guids 9090001-9090012).

## Validator result per spot (`.treasure check all`: 12 of 12 pass)
| Spawn | Spot | Position | Result | Cover | Path from road point |
|---|---|---|---|---|---|
| 9090001 | Burning Blade Coven (cave, Valley of Trials) | -132.6 -4315.2 66.1 | PASS 79 | indoors, 5/5 | 183 yd |
| 9090002 | Skull Rock, lower tunnels | 1489.1 -4821.8 8.6 | PASS 78 | indoors, 5/5 | 168 yd |
| 9090003 | Skull Rock, Felweaver Scornn's hall | 1468.1 -4692.9 6.5 | PASS 79 | indoors, 5/5 | 374 yd |
| 9090004 | Dustwind Cave | 915.2 -4748.2 20.1 | PASS 76 | indoors, 5/5 | 705 yd (2 legs) |
| 9090005 | Drygulch Ravine, harpy overhang | 993.1 -4680.1 46.5 | PASS 74 | overhang, 5/5 | 696 yd (4 legs) |
| 9090006 | Razorwind Canyon | 722.2 -4448.6 15.7 | PASS 80 | overhang, 5/5 | 307 yd |
| 9090007 | Razormane Grounds | 66.3 -4567.1 57.9 | PASS 68 | covered, 5/5, slope 22 | 360 yd |
| 9090008 | Tiragarde Keep, inside | -244.8 -5088.5 23.7 | PASS 79 | indoors, 5/5 | 253 yd |
| 9090009 | Scuttle Coast shipwreck | 226.7 -5141.2 2.3 | PASS 77 | covered, 5/5 | 49 yd |
| 9090010 | Echo Isles | -1127.8 -5134.2 4.2 | PASS 78 | covered, 5/5 | 354 yd |
| 9090011 | Northwatch Foothold | -973.0 -4480.0 25.7 | PASS 70 | covered, 5/5, in sight of road | 488 yd |
| 9090012 | Jaggedswine Farm | 1184.5 -4625.2 18.4 | PASS 76 | covered, 5/5 | 282 yd |

## How it was tested
- Unit tests: `ctest -R treasure` in `build/unit-tests` (7 tests).
- Regression: `treasure-1-loot` (Seedtest goes to 9090009, opens it, gets curio + green + gold, `explain` shows
  looted/hidden/not at client, second use refused) and `treasure-2-other` (Seedalt still sees it and can open it).
  Both pass on the Claude test env. Full suite not run.
- SQL applied to `auth_claude`, `characters_claude`, `world_claude`; the world file applied twice (idempotent).

## Decisions made
- Loot goes straight to the bags with a personal roll instead of the shared loot window, so friends can open the same
  chest at the same time. Full bags refuse the open and nothing is recorded.
- GMs in GM mode still see chests they looted (`Treasures.GmSeesLooted`).
- Under liquid fails by default; open sky is only a warning unless `Treasures.Validate.RequireCover = 1`.
- Skipped: Thunder Ridge (the ridge spots are all deep water in this map data), Deadeye Shore (open sky only),
  Darkspear Strand (best covered spot sits on an NPC), Rocktusk Farm (only partial cover).
- Placement-tool step 3 was provisional in DESIGN.md; implemented as described.

## For the other agent
- New config keys `Treasures.*` in `worldserver.conf.dist` (TREASURES section).
- New core hook header `GameObjectHooks.h` (not in `GameObject.h`): register hide/sparkle filters from a script.
- SQL: `sql/updates/characters/2026_11_05_claude_00_character_treasure.sql`,
  `sql/updates/master/auth/2026_11_05_claude_00_treasure_commands.sql`,
  `sql/updates/world/2026_11_05_claude_02_treasure_spot.sql`, `..._03_durotar_treasures.sql`.
- IDs: RBAC 99000, gameobject templates 909000-909011, references 909100-909105, spawn guids 9090001-9090012
  (row in `docs/ID_RANGES.md` comes with T-094).

## Next
- More zones with the same validator; an intro popup on first find exists (`Treasures.IntroOnFirstFind`), journal and
  achievement hooks once Phase 1b frameworks land.

## Map additions
- `src/server/scripts/Custom/Treasures/`, `tests/treasures/` (added to docs/MAP.md).
