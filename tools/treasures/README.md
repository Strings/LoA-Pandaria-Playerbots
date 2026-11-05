# Treasure placement tool

Puts hand-placed treasures into a zone without screenshots: candidate spots come from the spawn tables, the server's
`.treasure validate` (see `src/server/scripts/Custom/Treasures/README.md`) checks each one from map data, and the
zone file becomes an idempotent world SQL update.

## Files

| File | What it is |
|---|---|
| `treasure_zone.py` | `scan`, `check`, `settle` and `sql` commands |
| `themes.json` | Shared loot pools (one `reference_loot_template` entry each) plus the blue pool; reuse them across zones |
| `zones/<zone>.json` | One zone's treasures: name, chest display, area, position, road point, gold, curio, theme |

Environment: `MOP_SOAP_HOST/PORT/USER/PASSWORD` (your own test server), `MOP_DB_HOST/PORT/USER/PASSWORD`,
`MOP_WORLD_DB` and optionally `MOP_MYSQL_BIN`. Never point it at the human's server.

## Playbook for a new zone

1. **Pick landmarks.** Read the zone's areas (`AreaTable`, or `areaId` counts in `creature`) and choose 10 to 15 places
   with a story: caves, interiors, ruins, camps, wrecks, overhangs. Prefer places flying can't see into
   (DESIGN.md, Flying rule). Choose one road point per landmark: a spot on the nearest road or path, on the ground.
2. **Scan each landmark.** `treasure_zone.py scan <areaId> <roadX> <roadY> <roadZ>` validates the existing spawn spots
   in that area and ranks them; covered and indoor spots score highest. Mobs already stand where the interesting
   nooks are, so this finds them for free.
3. **Write the zone file** with the best spot per landmark (copy `zones/durotar.json`). Take the next free block from
   `docs/ID_RANGES.md` (Treasures row) for `entryBase` and `guidBase`.
4. **Check and settle.** `check zones/<zone>.json` lists failures and anything standing on a spot (an NPC, a quest
   object). `settle --write` moves each spot up to 3 yd to the best-covered passing offset, which also gets it off
   the mob it was borrowed from. Drop a landmark rather than force it: Thunder Ridge (flooded in the map data) and
   Deadeye Shore (open sky only) were dropped in Durotar.
5. **Loot.** Pick a theme per spot from `themes.json`, or add one for a new local faction or era with items from
   that era at the zone's level: one green pool, a curio per spot (grey or white flavour item), small gold. Raise
   `blueChance` for the hardest nooks.
6. **Generate and apply.** `sql zones/<zone>.json --out sql/updates/world/<date>_claude_NN_<zone>_treasures.sql`,
   apply it to your world database twice (it must be idempotent), restart, then `.treasure check all`: every spot
   should pass. Paste that output into your status fragment.
7. **Test** with the `treasure-1-loot` / `treasure-2-other` scenarios pointed at one of the new spawns if the zone
   adds anything new to the system; otherwise `.treasure check all` is enough.

Reference pools are shared: a zone file rewrites every pool it uses with the current `themes.json`, so a later
zone's update carries any pool changes forward.
