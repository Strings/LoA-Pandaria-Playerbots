# T-083: Heartseed active play, part B (Growth XP, no rank cap)

**Agent:** claude
**Branch:** claude/T-083b-growth-xp
**Task status:** done

## What changed
- The account rank and its level cap are gone. Minors fill in by character level (`min_rank` is a level now;
  all shipped minors are 1, so nothing moves). Feeding gives the appearance and essence rolls, no power.
- Growth XP per character: battleground wins (arenas excluded) and dungeon and raid boss kills near the
  character's level, for every credited group member. Each Growth level is a stat point; no cap.
- Stat points are spent by the player on Stamina, Main stat (strength, agility or intellect by spec), Critical
  strike or Haste, 1% per point (`heartseed_growth_stat`, server-side auras 310010 to 310015). Reset at a shrine.
- Addon: Growth line, stat buttons, Reset; sync `S` line changed, new `G` lines.
- GM: `.heartseed growth [+xp | level [xp]]`, `.heartseed points [spend | reset]`; player `.seed spend`,
  `.seed reset`. `.heartseed rank` and `.heartseed cap` removed.
- Core: `BattlegroundHooks` (small dispatch from `Battleground::EndBattleground`), so `ScriptMgr.h` is untouched.
- Capture tickets report Growth level, XP and unspent points instead of rank. Checkpoint `heartseed_rank` steps
  removed; `heartseed_growth` sets the Growth level.

## How it was tested
- Unit tests: Growth levels, near-level rule, unspent points, amounts, main stat by spec, minors by level.
- Addon tests (12), including spending only with points.
- Scenario `heartseed-gm` covers earning, spending, the stat aura, refusals and reset; full suite before the PR.
- SQL applied by hand to `characters_claude` and `world_claude`.

## Decisions made
- The user chose: minors by character level, and stat points picked by the player (2026-11-02).
- XP-locked characters (twinks) earn no Growth XP unless `Heartseed.Growth.TwinksEarn = 1`, so twink balance
  stays as DESIGN.md says.
- Stat list and 1% per point are placeholders; a reset is free but needs a shrine.

## For the other agent
- New SQL: `characters/2026_11_02_claude_03_heartseed_growth.sql`, `world/2026_11_02_claude_04_heartseed_growth.sql`
  (new `spell_dbc` rows: restart needed).
- Config: `Heartseed.Rank.*`, `Heartseed.Feed.GrowthBase`, `.GrowthPerItemLevel`, `.QualityPct` removed;
  `Heartseed.Growth.*` and `Heartseed.Feed.Qualities` added.
- Anything that wants a battleground result can register on `BattlegroundHooks`.

## Next
- Minor traits become small meter-feeding procs (DESIGN.md "Nothing from the monk kit").

## Map additions
- `src/server/game/Scripting/BattlegroundHooks.h/.cpp`: battleground result dispatch for custom systems.
