# T-097: Durotar treasures, second pass

**Agent:** claude
**Branch:** claude/T-097-durotar-treasures-2
**Task status:** review

## What changed
- 35 more Durotar treasures (47 in all), all passing, see docs/SECRETS.md. Most aren't chests: sacks, jugs, bones,
  clams, books, crates and baskets, placed in camps, caves, towns and the coast.
- `treasure_spot.sparkle` (default 1): 0 means the object is hidden in plain sight and never sparkles, even with the
  Heartseed. Seven spots use it.
- Seven treasure items (199000-199006): five blue trinkets and two epics (a trinket and a dagger) that carry
  scaled-down effects from later expansions (Hex, Thorns, water walking, slow fall, shadowflame, fire nova, a fel
  chance-on-hit). Each drops from exactly one spot. Spells 319000-319006 are client clones.
- Four new loot themes in `tools/treasures/themes.json` (Valley of Trials, Horde military, coast, Northwatch).
- `treasure_zone.py`: `scan --near X Y R` for areas with shared or wrong area ids, `settle --clear R` to push spots
  off NPCs and objects, per-spot `extra` named drops and `sparkle`.
- `tools/clientpatch` spell clones take `duration_index` and per-effect overrides (`base_points`,
  `points_per_level`, `aura`, `misc_value`, `trigger_spell`, `radius_index`).

## How it was tested
- TESTING

## Decisions made
- Treasure trinkets don't share a cooldown category; each has its own cooldown.
- Felweaver's Fang procs at 2.5 per minute (`item_template_addon.SpellPPMChance`).

## For the other agent
- Apply in order: `2026_11_05_claude_12_treasure_spot_sparkle.sql`, the regenerated
  `2026_11_03_claude_00_heartseed_gift_spells.sql`, `..._13_treasure_items.sql`, `..._14_durotar_treasures.sql`.
  The generated zone SQL needs file 12 first.
- The client patch has seven new spells; reinstall it on deploy or the items show unknown spells.
- Spell clones carry no effect scaling, but `points_per_level` still applies: zero it for flat amounts.

## Next
- More zones with the playbook in `tools/treasures/README.md`.

## Map additions
- None.
