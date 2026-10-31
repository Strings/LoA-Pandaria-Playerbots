# T-048 (part 2): twink checkpoints

**Agent:** claude
**Branch:** claude/T-048-twink-kits
**Task status:** review

## What changed
- New checkpoint step `twink_gear`: best twink gear for the target's class, spec and level, chosen at apply time
  rather than from hand-written kits, so one step covers every class at every bracket.
  - The candidate pool is `gmtools_twink_candidate` plus T-045's tuned `twink_item`. The 3,565 candidates come
    from T-045's shortlist query, widened to the top two per primary stat. `tools/gmtools/twink_candidates.py`
    regenerates them.
  - Selection rules live in `TwinkGearRules` (pure logic, unit test `unit_twink_gear`): armor type, primary
    stat, weapon style per spec, and an outlier filter.
- Nine twink checkpoints: `twink-19`, `-29`, `-39`, `-49`, `-59`, `-69`, `-79`, `-84` and `-89`.
- Equipping is shared with kits:
  - A copy already in the bags is equipped before a new one is made.
  - Replaced gear goes to the bags.
  - Slots that twink gear manages but has no pick for are emptied.
  - Gear the character can no longer use after a level-down is unequipped.

## How it was tested
- `unit_twink_gear`: profiles, armor and stat choice, rings and trinkets, weapon styles, outliers, fallbacks.
  Passes, with no `/W4` warnings.
- Headless, `Seedtest`, a warrior. Before applying, I applied Codex's twink SQL and both new SQL files twice to
  `world_claude`.
  - `twink-29`: 13 of 13 items equipped from 469 usable candidates, mail and strength items.
  - The same checkpoint again: nothing new equipped, and the bag count is unchanged.
  - `twink-19` after `twink-29`: 7 unusable items moved to the bags, 11 of 11 equipped.
  - `twink-59`: 15 of 15, plate Darkcrest set.
- Built with no new warnings.

## Decisions made
- **Selection at apply time instead of per-class kit rows.** With 11 classes, 34 specs and 9 brackets, kit rows
  would be hundreds of hand-picked lists. Kits stay for fixed bundles.
- **Outlier filter:** anything over 115% of the best leveled item level within ten levels. Without it, toys, PvP
  pieces and quest rewards with no level requirement (item level 85 to 400 at bracket 29) won.
- **Known roughness:**
  - Neutral-stat items (a fishing trinket, for example) can still win a slot when nothing better exists.
  - Proc and socket value is ignored, the same limits as T-045.

## For the other agents
- Apply `2026_10_31_codex_00_twink_gear.sql` before the twink checkpoints, so the tuned item levels are in.

## Next
- Playtest the brackets and tune the rules if a class looks wrong.
