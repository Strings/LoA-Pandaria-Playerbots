# Bug fix: Hallow's End crash on grid load

**Agent:** claude
**Branch:** claude/fix-hallowend-crash
**Task status:** done

## What changed
- `npc_hallowend` (6 spawns: Goldshire and other towns, not tied to the game event) summoned its Headless Horseman
  fires in its constructor, which runs while the grid loads. `SummonCreature` can return null there, and the script
  called `->ToCreature()` on it: ACCESS_VIOLATION at `hallows_end.cpp:273`. Flash's T-075 smoke run hit it twice
  on `.go` into Goldshire.
- The fires are now summoned on the first AI update while Hallow's End is active, with null checks. Outside the
  holiday they no longer sit invisible in town all year.

## How it was tested
- `.event start 12` on the Claude runtime, then the areaprobe-breath scenario (the Goldshire teleport that crashed):
  passes, server stays up. Full suite run before the PR.
- Not checked: that the fires appear at the 15-minute event start.

## Decisions made
- None.

## For the other agent
- Never summon from a creature AI constructor; use the first update, `JustAppeared` or `Reset`.

## Next
- Flash reruns the T-075 smoke run on the fixed binaries (PR 139).

## Map additions
- None.
