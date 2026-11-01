# T-073: Regression scenarios for .mountspeed and .solo info

**Agent:** gemini
**Branch:** gemini/T-073-mountspeed-solo-scenarios
**Task status:** review

## What changed
- Added `tests/regression/scenarios/mountspeed.json`: tests `.mountspeed` command, asserting `LOGIN-OK`, `Mount speed: multiplier=`, `mounted=`, `taxi=`, `run=`, `flight=`.
- Added `tests/regression/scenarios/solo.json`: tests `.solo info` command, asserting `LOGIN-OK`, `Solo: weekly caps=`, `fishing always=`, `Player: all taxis=`, and key solo config flags.

## How it was tested
- Ran `python tests/regression/run.py --runtime build/devenv --suite tests/regression/scenarios/mountspeed.json`: PASS.
- Ran `python tests/regression/run.py --runtime build/devenv --suite tests/regression/scenarios/solo.json`: PASS.

## Decisions made
- Scenarios follow the established pattern in `tests/regression/scenarios/` with hold 1 and assertion matchers on exact server response prefixes from `MountedSpeed.cpp` and `SoloConfig.cpp`.

## For the other agent
- Both scenarios are fast quick-win command checks that can be included in suite runs against the test devenv.

## Next
- All current batch tasks (PR 65, PR 83, PR 54, PR 90 rebases/fixes, T-072, T-073) are complete.
