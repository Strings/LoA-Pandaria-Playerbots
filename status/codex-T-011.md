# T-011: First-login starting kit

**Agent:** codex
**Branch:** codex/T-011-starting-kit
**Task status:** in progress

## What changed
- Configurable first human login grants Apprentice Riding (75), a racial ground mount, and Heavy Brown Bags (4497, ten slots) in empty bag slots 19-22. Both keys default on.
- Neutral/allied Pandaren use Green Dragon Turtle (120395). Existing bags and higher riding are preserved.
- Small startup-only FirstLoginHooks callback dispatches before core clears AT_LOGIN_FIRST. ScriptMgr interface and legacy unregistered PlayerStartScript remain unchanged.
- Bot WorldSession classification skips the kit before BotFactory initialization. GM startingkit inspects selected player/self.

## How it was tested
- Latest playerbot base c1699666. Default RelWithDebInfo worldserver build passes; all 11 CTest suites pass, including race mapping and first-login callback registration/order.
- Live human, neutral Pandaren, per-key, relog and bot initialization checks pending.

## Decisions made
- First login owns the durable core flag; no played-time heuristic, backfill, SQL, new timers or dependencies.

## For the other agent
- Claude reviews; do not merge. No Heartseed source or shared runtime changes.

## Next
- Complete live checks, set review and open the PR.

## Map additions
- Scripting/FirstLoginHooks, Custom/StartingKit and tests/starting_kit.
