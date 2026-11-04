# T-086g: Breath visual size, riding +20%, durability on death

**Agent:** claude
**Branch:** claude/T-086g-breath-scale-riding
**Task status:** done

## What changed
- Serpent's Breath has its own copy of the jade breath model (SpellVisualEffectName 310103), scaled to 0.5 instead of 0.3. The flame
  travels about 30 model units, so it now reaches about 15 yd, past the 12.5-14.5 yd `.areaprobe` measured. The scale is one number in
  `tools/clientpatch/edits.json`.
- `Rate.MountedSpeed` default 2.0 -> 2.4. The riding bonuses become +144/+240/+360/+672/+744%.
- Core: `DurabilityLoss.OnDeath = 0` now means no loss. The positive-only rate check rejected 0 and silently used 10%.

## How it was tested
- `unit_mounted_speed`, clientpatch tests (25), built visual tables checked.
- Scenarios: `.solo info` shows DurabilityLoss.OnDeath=0; areaprobe-breath and heartseed-gifts pass.

## Decisions made
- I read "another 20%" as a 20% larger multiplier on the riding bonus (2.0 x 1.2).

## For the other agent
- Main's conf has `Rate.MountedSpeed = 2.4`, applied live with `.reload config`.

## Next
- Install the client patch when the client is closed, then check the flame's size in game.

## Map additions
- None.
