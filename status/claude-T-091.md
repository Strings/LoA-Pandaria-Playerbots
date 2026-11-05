# T-091: Heartseed Gift at level 1, Gift probe

**Agent:** claude
**Branch:** claude/T-091-heartseed-level1
**Task status:** done

## What changed
- The first major tier (the Celestial Gift) and the first essence slot open at level 1:
  `Heartseed.Majors.TierLevels = "1 25 40 55 70 85"`, `Heartseed.Essence.SlotLevels = "1 40 70"`.
- Turning in the granting quest opens the Gift choice after `Heartseed.Choice.PromptDelay`.
- Niuzao's Retort shield moves from its Sprout rider (stage 1) to stage 0, so it comes with the Gift.
- `.heartseed probe [name]` (console too): each Gift's output for any online character against its white hit and
  class filler spell. `.heartseed major` usage lists the configured tier levels.

## How it was tested
- Unit tests 24/24 (defaults updated). New scenario `heartseed-level1`; all Heartseed scenarios pass (scenarios
  now pass tier level 1 to `.heartseed major`; the riders scenario expects the shield at stage 0).

## Decisions made
- Human request (2026-11-05): Heartseed power from level 1, to speed up solo progression. Gifts already scale with
  the character's own power, so no low-level tuning was needed.

## For the other agent
- Apply `sql/updates/world/2026_11_05_claude_00_heartseed_gift_at_level1.sql`.
- Runtime confs that set the old levels explicitly need the new values (devenv confs from `setup.py` do).

## Next
- Measure the Gifts with the probe across levels and classes, then tune (melee vs caster power, Retort floor).
- Growth redesign (DESIGN.md).

## Map additions
- None.
