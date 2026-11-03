# T-086f: Gift polish (tooltips, breath visual and reach, area probe)

**Agent:** claude
**Branch:** claude/T-086f-gift-polish
**Task status:** review

## What changed
- Client patch: Gift tooltips show the server's cooldowns (own SpellCooldowns rows). Serpent's Breath loops a jade
  dragon-breath visual from the mouth. `tools/clientpatch` gained `row` edits, which copy a DBC or DB2 row to a new id.
- Serpent's Breath reaches `Heartseed.Gift.BreathRange` (12 yd) for players. The creature cone it borrows reached 100 yd.
- New GM tool `.areaprobe place|report|reset|clear` (`src/server/scripts/Custom/GMTools/AreaProbe/`). It stands
  rooted probe dummies on rings and spokes, records which spells touch them, and fits reach, cone angle and circle
  from the hits and misses. This measures spell areas with no one in game.

## How it was tested
- Unit tests: `unit_area_probe` (8 fits), clientpatch Python tests.
- Scenario `areaprobe-breath`: Serpent's Breath measures as a 120 degree cone, reach 14-16 yd to probe centers
  (12.5-14.5 yd to the probe's edge). Close in, its hits widen to about 90 degrees (the core's 3 yd allowance for players).

## Decisions made
- Breath reach of 12 yd is my call; it is a config key.

## For the other agent
- New permission 91010 (`sql/updates/master/auth/2026_11_03_claude_00_areaprobe_command.sql`).
- New config keys: `Heartseed.Gift.BreathRange`, `GMTools.AreaProbe.Lifetime`, `GMTools.AreaProbe.MaxProbes`.
- Use `.areaprobe` to check any AoE you add. Choose a spoke count that doesn't land on the cone edge, such as 25.
- Always launch the client with `_Wow-64.exe`: the stock exe verifies files and crashes on our patch.

## Next
- Size the breath visual to the measured reach (the attach row's `scale`); the probe measures targeting, not the visual.

## Map additions
- `src/server/scripts/Custom/GMTools/AreaProbe/`: spell area probe (GM tool).
