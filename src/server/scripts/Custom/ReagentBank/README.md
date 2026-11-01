# Reagent bank (T-043)

Crafting reagents kept outside the bags, in detached storage (`Detached::Kind::ReagentBank`, container 0, one slot
per stack). Spells take reagents from the bags first, then from the bank, so every profession crafts straight from it,
anywhere.

## Pieces

- `ReagentBankRules.h`: pure stack rules (unit tested in `tests/reagent_bank/`). Deposits top up existing stacks in slot
  order before opening a new one; use and withdrawal drain the smallest stacks first.
- `ReagentBank.h/.cpp`: deposit, deposit all, withdraw, use, GM add, contents.
- `reagentbank_scripts.cpp`: the `.reagent` commands.
- Core hook: `src/server/game/Spells/ReagentSource.h`. `Spell::CheckItems`, `Spell::TakeReagents` and the
  missing-reagent cast result ask it instead of `HasItemCount` / `DestroyItemCount`. With no source registered, the
  core behaves as stock.
- Addon: `addons/ReagentBank/`.

## What counts as a reagent

Any item some spell lists as a reagent (built from the spell store at startup), except quest items, unique items and
items with a duration: their counts must stay in the bags for quests and unique checks to see them.

## Commands

| Command | Who | Does |
|---|---|---|
| `.reagent` / `.reagent list` | player | Lists the bank. |
| `.reagent deposit` | player | Deposits every reagent in the backpack and bags. |
| `.reagent deposit <bag 0-4> <slot 1+>` | player | Deposits one stack. |
| `.reagent take <item id or link> [count]` | player | Takes items back to the bags; count defaults to one stack. |
| `.reagent sync` | player | Addon data: `R\t<item>\t<count>` per kind, then `E`. |
| `.reagent inspect` | GM | Selected player's bank, stack by stack. |
| `.reagent add <item id or link> [count]` | GM | New items straight into the selected player's bank. |

RBAC 93002 (player, role 199) and 93003 (GM, role 197).

## Config

- `ReagentBank.Enable` (1): off leaves the contents in place; crafting stops drawing from the bank.
- `ReagentBank.MaxStacks` (0 = no limit).

## Notes

- Withdrawals create fresh items rather than handing back the stored ones, so they merge into bag stacks like loot.
  Reagents carry nothing per item worth keeping.
- Bank use during a cast commits at once; the crafted item is saved with the next player save, as stock crafting is.
- Bots have no bank (detached storage is not loaded for them).
