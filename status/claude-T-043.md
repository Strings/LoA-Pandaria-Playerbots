# T-043: Reagent bank in detached storage, with crafting drawing from it

**Agent:** claude
**Branch:** claude/T-043-reagent-bank
**Task status:** review

## What changed
- `src/server/scripts/Custom/ReagentBank/`: the bank. Stacks live in detached storage (kind ReagentBank, container 0,
  one slot per stack). Deposits top up existing stacks first; crafting and withdrawals drain the smallest stacks
  first (`ReagentBankRules.h`, unit tested).
- Core hook `src/server/game/Spells/ReagentSource.h/.cpp`: `Spell::CheckItems`, `Spell::TakeReagents` and the
  missing-reagent cast result count bags plus the registered source, and take from the bags first. Three call
  sites changed in `Spell.cpp`; with nothing registered the core behaves as stock.
- `DetachedStorage`: `SetCount` and `Destroy` for stack merging and use.
- Commands: `.reagent [list|deposit [bag slot]|take <item> [count]|sync]`, GM `.reagent inspect`, `.reagent add`.
  RBAC 93002/93003 (`sql/updates/master/auth/2026_11_01_claude_03_reagent_command.sql`).
- Config: `ReagentBank.Enable = 1`, `ReagentBank.MaxStacks = 0` (no limit).
- Addon `addons/ReagentBank/`: `/rb` panel, a button on the profession window, bank counts in item tooltips, and a
  one-time intro popup on the first profession window (the system's introduction).

## How it was tested
- Unit tests: 14/14 suites, including `unit_reagent_bank_rules`.
- Live on the claude server with the headless client: see the PR for the steps and output.

## Decisions made
- Character scope, not account: the design doesn't say, per-character matches the era, and account-scope detached
  items still die with their owning character (DetachedStorage README).
- What counts as a reagent: any item some spell lists as a reagent, except quest, unique and timed items.
- Usable anywhere (no banker needed), no stack limit by default. Both are easy to change (a check, a config value).
- Any spell's reagents draw from the bank, not only profession spells.

## For the other agent
- Apply the auth SQL to your auth DB to get `.reagent`.
- `ReagentSource` is the place for any other off-bag reagent source; there is one source slot.

## Next
- Playtest in the real client: whether the profession window lets you press Create when the reagents are only in
  the bank (docs/PLAYTEST.md). If the client blocks it, add a craft-from-bank button to the addon.

## Map additions
- `src/server/scripts/Custom/ReagentBank/`, `src/server/game/Spells/ReagentSource.h`, `addons/ReagentBank/`.
