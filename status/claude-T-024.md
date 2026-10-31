# T-024: Heartseed GM commands (rank, Growth, story flags)

**Agent:** claude
**Branch:** claude/T-024-heartseed-gm (stacked on T-020, PR #9)
**Task status:** review

## What changed
- `.heartseed rank <1-255>`, `.heartseed growth <n> | +<n> | -<n>`,
  `.heartseed flag list | set <name|id> | clear <name|id>`. `.heartseed info` lists story flags by name.
- `HeartseedMgr::SetRank`, `SetGrowth`, `SetStoryFlag` through one `ModifyState` helper: changes save at once
  and refresh the glow. T-022 (feeding) and T-026 (story hooks) use the same calls.
- Story flag names (`first_dungeon`, `reached_pandaria`, `first_world_boss`); any other number is a valid
  flag too, so new beats need no code to be set by hand.

## How it was tested
Claude test server, headless client, character `Seedtest` (level 30), steps through `say .heartseed ...`:
- every command refuses a character without a Heartseed;
- `rank 0` rejected, `rank 7` set;
- Growth: set 100, +50 = 150, -500 clamps to 0, +1234;
- flags: set by name, by number (3) and an unknown id (42); `bogus` rejected; clear by name and number;
- database after the session: rank 7, Growth 1234, flags 1 and 3 with set times;
- relog: rank, Growth and flags reload; `revoke` clears everything.

## Decisions made
- GM rank ignores the level cap (the cap comes with T-022), so later ranks can be tested at any level.
- Setting a flag that is already set keeps its first time.

## For the other agent
- No SQL. Rebuild only touches `Custom/Heartseed/`.

## Next
- T-021 (traits and core passive) and T-022 (feeding, rank cap) build on these setters.

## Map additions
- None.
