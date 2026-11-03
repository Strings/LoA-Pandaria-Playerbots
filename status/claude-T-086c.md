# T-086c: Heartseed panel fixes, stat curve, shrine rule off, MPQ v2 archives

**Agent:** claude
**Branch:** claude/T-086c-panel
**Task status:** review

## What changed
- Addon: the Gift drags to action bars; Gift and meter icons retry on `SPELLS_CHANGED`; layout no longer overlaps (mandala lower, essence list under the majors' level labels, footer follows the essences, frame grows); Gift description wraps; stat icons show their bonus.
- Growth stats have diminishing returns: `max_pct * points / (points + half_points)`, nearest percent (`Heartseed::StatPointAmount`). Stamina 30/20, main stat 20/20, crit 10/10, haste 10/10. The `G` sync line now carries current bonus, next point's gain and ceiling.
- `Heartseed.Choice.ChangeAtShrineOnly` and `Heartseed.Growth.ResetAtShrineOnly` default to 0.
- `tools/clientpatch/mpq.py` writes StormLib's version 2 layout: 44-byte header, 4 KB sectors, `(attributes)` with CRC32 and MD5. Version 1 archives made the client's repair-on-demand fail on SpellEffect.dbc (ERROR #134), compressed or not.

## How it was tested
- Unit: `unit_heartseed_rules`, clientpatch 21/21, addon 14/14.
- Scenarios: heartseed-gifts, -gm, -meters, -riders, -sync, -xuen pass.
- StormLib (`fusion-mpq extract`) reads every file of the new archives back byte for byte. Real client launch: pending the human's playtest.

## Decisions made
- The human chose the diminishing curve and dropped the shrine requirement (2026-11-03); DESIGN.md updated.
- Crit and haste reach half their ceiling at 10 points, so the first point shows +1% (the core's percent auras take whole numbers).

## For the other agent
- Apply `sql/updates/world/2026_11_03_claude_02_heartseed_growth_curve.sql` (rebuilds `heartseed_growth_stat`; `pct_per_point` is gone, now `max_pct` and `half_points`).
