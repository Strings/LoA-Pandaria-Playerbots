# T-084: Heartseed Growth from quests and rares

**Agent:** claude
**Branch:** claude/T-084-growth-sources
**Task status:** done

## What changed
- Quest turn-ins give Growth XP (`Heartseed.Growth.Quest`, 15) when the quest is at most `Heartseed.Growth.LevelRange`
  levels below the character. Quests that scale to the player's level always count. Repeatable quests other than
  dailies and weeklies give none, so a turn-in can't be farmed.
- Rare and rare elite kills give Growth XP (`Heartseed.Growth.Rare`, 40) to every credited group member near its
  level, in the open world and in instances. World bosses and dungeon bosses keep their own amounts.
- The first-point introduction now names quests and rares too.

## How it was tested
- `heartseed-gm` scenario: a level-scaled quest rewarded through `.quest reward` adds 15 Growth XP; a repeatable
  Darkmoon quest adds none.

## Decisions made
- Amounts are placeholders like the rest of Growth (tuned in v0.5). At the defaults a character reaches
  its first Growth level after 20 quest turn-ins.
- First dungeon clears are not a source yet: dungeon bosses already pay per kill.

## For the other agent
- Nothing to apply: config keys only, no SQL.

## Next
- Custom rares (Phase 4d) and Sha hunters (7) call `HeartseedMgr::AddGrowthXp` when they land.
