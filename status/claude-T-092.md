# T-092: Gift power rework

**Agent:** claude
**Branch:** claude/T-092-gift-power
**Task status:** done

## What changed
- Gift power = `Heartseed.Gift.LevelBasePct` (122) % of the gtSpellScaling value for the level (one row for every
  class, `Heartseed.Gift.ScalingClass` = 8) + max(spell power, attack power x `AttackPowerPct` (80) %). Pure rule
  `Heartseed::GiftPower` with unit tests.
- Serpent's Breath tick: 68% of the Gift power (was 38% of the higher of AP and SP).
- Niuzao's Retort deals at least `Heartseed.Gift.RetortFloorPct` (200) % of the Gift power per enemy.
- `.heartseed probe` shows the Gift power and the Retort floor, compares melee with a normalized weapon strike
  and finds the filler's damage effect (Frostbolt read 0 before).

## How it was tested
- Unit tests 24/24. heartseed-level1 (now asserts a nonzero Gift power), heartseed-gifts, -meters, -riders,
  -choose, areaprobe-breath, breath-move pass. Probed on main's bots before and after (numbers in the PR).

## Decisions made
- Targets from DESIGN.md "Gift power targets" (proposed): empty Breath about 2x the filler, full meter 4x.
  Coefficients fitted to probe data at levels 9 to 90.

## For the other agent
- Apply `sql/updates/world/2026_11_05_claude_01_heartseed_gift_power.sql`.

## Next
- Xuen's tiger damage is still the guardian's own stats; measure in play.

## Map additions
- None.
