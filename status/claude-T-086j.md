# T-086j: Serpent's Breath while moving, faster flight paths

**Agent:** claude
**Branch:** claude/T-086j-breath-move-taxi
**Task status:** done

## What changed
- Serpent's Breath keeps channeling while you move and turn. The clone gets its own SpellInterrupts row with the
  move and turn bits cleared (new `interrupts` key on clientpatch `spell` edits); the server, whose data files are
  stock, clears the same bits in `SpellMgr::LoadSpellInfoCorrections` (case 310103).
- `Taxi.SpeedOfTopFlight` (default 2.0): flight paths fly that many times as fast as Master Riding flying through
  `Rate.MountedSpeed` (about 118 yd/s at 2.4, against the stock 32). The spline speed cap (50 yd/s) skips units in
  flight. `.mountspeed` prints the flight-path speed.

## How it was tested
- clientpatch and `unit_mounted_speed` unit tests; scenarios breath-move (new), taxi-speed (new), mountspeed,
  ghost-speed, heartseed-gifts, areaprobe-breath.

## Decisions made
- "Highest player flight speed" is Master Riding (310%) scaled by `Rate.MountedSpeed`; faster temporary buffs don't count.

## For the other agent
- `MoveSplineInit` no longer caps velocity for units with `UNIT_STATE_IN_FLIGHT`.

## Next
- Watch a long flight path in game for map loading at the new speed.

## Map additions
- None.
