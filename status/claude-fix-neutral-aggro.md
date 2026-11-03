# Bug fix: neutral mobs don't fight back

**Agent:** claude
**Branch:** claude/fix-neutral-aggro
**Task status:** done

## What changed
- `Creature::_IsTargetAcceptable` also accepts a target that is already on the creature's threat list
  (`IsEngagedBy`), as TC master does. Before this, a neutral (yellow) mob accepted a player only if the
  player's own victim was the mob. A caster who never starts auto-attack has no victim, so the threat
  reference went offline as soon as it was made (`ThreatReference::ShouldBeOffline`, from the
  2026-10-23 ThreatManager port). The mob took the damage, never engaged and evaded back to full health.
  Melee players and hostile (red) mobs were unaffected.

## How it was tested
- Reproduced on the Claude test server with a level-2 troll mage: Frostbolt took a Mottled Boar from
  42 to 19 HP, no ATTACKSTART, the boar stayed put and reset to 42.
- New scenario `tests/regression/scenarios/neutral-aggro.json` (Seedtest with troll faction, `.damage`
  without auto-attack): fails on the old build, passes on the fixed one.

## Decisions made
- None.

## For the other agent
- Hostile and melee cases were never broken; casters (mage, warlock, priest, hunters at range) at
  neutral mobs were.

## Next
- None.

## Map additions
- None.
