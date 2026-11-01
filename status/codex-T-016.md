# T-016: Free respecs and dual spec from ten

**Agent:** codex
**Branch:** codex/T-016-free-respec
**Task status:** review

## What changed
- Existing NoResetTalentsCost defaults on and zeroes both talent/spec reset quotes and charges.
- Respec.FreeReagents defaults on, narrowly using REMOVE_TALENT/APPLY_GLYPH effects in CanNoReagentCast so validation and consumption agree. Other spells/reagents remain normal.
- Respec.AutoDualSpec defaults on; existing MinDualSpecLevel defaults to 10. Human login/level-up uses standard trainer learning spells; bot groups stay owned by BotFactory.
- GM respec info/reset/learn/cast commands and module README. No new durations, SQL or dependencies.

## How it was tested
- Release worldserver build passes after rebase onto 970163d2, no new warnings in touched files; all eight CTest suites pass, including unlock boundaries/disabled/bot cases.
- Live Codex test/none: talent and spec resets charge zero; disabled cost charges 10000 copper. Normal talent removal succeeds with no reagent only when enabled.
- Valid minor glyph 490 in slot 0: absent reagents reject removal with SPELL_FAILED_REAGENTS when disabled; a supplied Tome is consumed normally. Enabled removal consumes none, with or without a Tome held.
- Fresh level-9/single-spec fixture stays single; real level-up to 10 unlocks both. Normal 63644 cast switches active spec to 1 at level 10. Disabled/re-enabled auto unlock and relog verified.
- Headless custom-reload-rng/login regression passes. Shattrath NPC multiline dialogue hit an existing runner parser limitation; moved to a quiet fixture location and reran successfully without changing the runner.
- Evidence: build/T016-*.log and build/regression/T016-quiet and T016-integrated.

## Decisions made
- Reused core keys and normal learning/spell paths; no provisional features or extra loadout system.

## For the other agent
- Claude reviews; do not merge. No Heartseed source/shared-runtime changes.
- Disabling auto unlock does not revoke existing dual spec. All spell level/combat/challenge restrictions remain normal.

## Next
- Claude review, human merge.

## Map additions
- World/RespecConfig caches new toggles. Custom/Respec and tests/respec hold adapter, commands, README and pure unlock policy.
