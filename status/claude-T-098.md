# T-098: Second-class track 10 to 90

**Agent:** claude
**Branch:** claude/T-098-multiclass-track
**Task status:** done

## What changed
- Second classes open at character level 10 (`Multiclass.UnlockLevel`, was 30).
- `Multiclass.XpPct` 275 (was 25): the 30-level track spans character levels 10 to 90.
- First unlocks moved to second-class levels 1/3/5 (were 1/5/10), rules' `min_level` with them
  (`sql/updates/world/2026_11_06_claude_00_multiclass_track.sql`).
- DESIGN.md: about 14 unlocks per class, one every two levels: about 8 actives and 6 impactful passives or procs,
  not only damage.

## How it was tested
- `multiclass` and `multiclass-necromancer` scenarios; `multiclass` now takes the Barbarian at character level 10.

## Decisions made
- Human (2026-11-06): unlock at 10, the slower curve, roughly 8 abilities plus passives and procs. Slower overall
  levelling is tabled until the baseline has been played.

## For the other agent
- Main's `worldserver.conf` sets no `Multiclass.*` keys, so the new code defaults apply there.
- `multiclass` fails with cast result 50 (line of sight) when Seedtest was left on the Durotar ledge at
  (-250, -3976, 159). An in-scenario `.tele` near teleport does not move the headless client; move the character
  offline first: `soap.py ".tele name Seedtest RazorHill"`. Worth a KNOWLEDGE.md line at the fold.

## Next
- T-099: fill both tracks.
