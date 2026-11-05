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
- 12 Durotar treasures in `sql/updates/world/2026_11_05_claude_07_durotar_treasures.sql` (templates 909000-909011,
  reference pools 909100-909105, spawn guids 9090001-9090012).

## Validator result
- `.treasure check all`: 12 of 12 pass, all with full cover overhead (scores 68-80). Where each one is, and its
  hint ladder, is in `docs/SECRETS.md` (spoilers).
- `tools/treasures/treasure_zone.py` + `zones/durotar.json` now generate the Durotar SQL; README there is the
  playbook for the next zone.

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
- The sealed secrets log is `docs/SECRETS.md` (the human asked for it; the human prefers not to know where things
  are). Treasure locations live only there, not in this fragment or STATUS.md.
- DESIGN.md gains two provisional blocks under Rares and discovery, at the human's request: secret patterns taken
  from WoW Forever (ideas only) and zone lore tiers (25/50/75/100% permanent bonuses, from the wowfusion design).

## For the other agent
- New config keys `Treasures.*` in `worldserver.conf.dist` (TREASURES section).
- New core hook header `GameObjectHooks.h` (not in `GameObject.h`): register hide/sparkle filters from a script.
- SQL: `sql/updates/characters/2026_11_05_claude_04_character_treasure.sql`,
  `sql/updates/master/auth/2026_11_05_claude_05_treasure_commands.sql`,
  `sql/updates/world/2026_11_05_claude_06_treasure_spot.sql`, `..._03_durotar_treasures.sql`.
- IDs: RBAC 99000, gameobject templates 909000-909011, references 909100-909105, spawn guids 9090001-9090012
  (row in `docs/ID_RANGES.md` comes with T-094).

## Next
- More zones with the same validator; an intro popup on first find exists (`Treasures.IntroOnFirstFind`), journal and
  achievement hooks once Phase 1b frameworks land.

## Map additions
- `src/server/scripts/Custom/Treasures/`, `tests/treasures/` (added to docs/MAP.md).
