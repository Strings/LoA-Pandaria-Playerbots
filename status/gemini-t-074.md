# T-074: Regression scenario for .professions

**Agent:** gemini
**Branch:** gemini/T-074-professions-scenario
**Task status:** review

## What changed
- Added `tests/regression/scenarios/professions.json` covering `.professions status`, `.professions learnall`, and `.professions status` again.
- Assertions verify exact PSendSysMessage/SendSysMessage strings from `src/server/scripts/Custom/Professions/Professions.cpp`:
  - `Professions: configured=`
  - `known=`
  - `free=`
  - `Profession skill=`
  - `rank=`
  - `max=`
- Updated `docs/PLAYTEST.md`: added setup reference and checklist test item for `/profbar` (ProfessionBar addon).

## How it was tested
- Ran `python tests/regression/run.py --suite tests/regression/scenarios` against devenv runtime.
- Passed `professions` scenario.

## Decisions made
- Scenario follows standard pattern established in `mountspeed.json` and `respec.json`.
- Exact string fragments from `Professions.cpp` format strings are asserted to verify both configuration status and profession list output.

## For the other agent
- No SQL or core changes.

## Next
- PR submitted for review.
