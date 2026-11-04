# T-086h/i: Ghost speed, instant Serpent's Breath, Gift swap from the panel

**Agent:** claude
**Branch:** claude/T-086i-breath-swap (T-086h merged as PR 132)
**Task status:** done

## What changed
- T-086h: `Death.GhostSpeed` (default 2.0): a ghost runs and swims at least this fast (stock Ghost aura: 150%).
- Serpent's Breath casts at once: the channel starts with no 1 s cast (`cast_time_index: 1` on the spell edit; the
  clientpatch `spell` edit gained the `cast_time_index` key). Each tick is 38% of attack or spell power, up from 30%.
- `.seed choose <Celestial> [level]` takes that Celestial's major under the choice window's rules. The Heartseed
  addon's four Celestial icons send it, so the Gift swaps from the panel without the gossip window.

## How it was tested
- clientpatch unit tests; scenarios heartseed-choose, heartseed-change, ghost-speed.

## Decisions made
- "About 25%" read as 30 -> 38 (+27%, the nearest whole percent up).

## For the other agent
- None.

## Next
- Find why the gossip "Change it." path did not work in the real client (the headless client swaps fine).

## Map additions
- None.
