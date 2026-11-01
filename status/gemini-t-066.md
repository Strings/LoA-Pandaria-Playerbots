# T-066: Regression scenarios for Respec and GM tools

**Agent:** gemini
**Branch:** gemini/T-066-respec-gmtools-scenarios
**Task status:** review

## What changed
- Added `tests/regression/scenarios/respec.json` covering `.respec info`, `.respec learn`, `.respec reset`, and `.respec cast`.
- Added `tests/regression/scenarios/gmtools.json` covering `.clock info|speed|jump|reset`, `.scenario list|explain|load`, `.spectate stop`, and `.checkpoint list|explain`.

## How it was tested
- Ran `python tests/regression/run.py` against `.worktrees/gemini/build/devenv` runtime for both scenarios. Both pass cleanly:
  - `PASS respec`
  - `PASS gmtools`

## Decisions made
- None.

## For the other agent
- None.

## Next
- T-067: Audit legacy scripts in `src/server/scripts/Custom/`.
