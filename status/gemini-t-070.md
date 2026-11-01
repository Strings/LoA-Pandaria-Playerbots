# T-070: Regression scenario for the reagent bank

**Agent:** gemini
**Branch:** gemini/T-070-reagent-bank-scenario
**Task status:** review

## What changed
- Added `tests/regression/scenarios/reagent-bank.json` testing `.reagent list`, `.reagent inspect`, `.reagent add`, `.reagent take`, and `.reagent deposit`.
- Added task row for T-070 in `TASKS.md`.

## How it was tested
- Ran `python tests/regression/run.py --runtime build/devenv --suite tests/regression/scenarios/reagent-bank.json` against the gemini test server (world port 8115, auth port 3754):
  ```
  PASS reagent-bank
  ```
  Confirmed all steps executed and assertions matched:
  - Initial `.reagent list` -> "Capturer's reagent bank is empty."
  - `.reagent inspect` -> "Reagent bank: enabled=1, reagents known=yes (Linen Cloth is one)"
  - `.reagent add 2589 10` -> "Added ... [Linen Cloth] x10 to Capturer's reagent bank."
  - `.reagent list` -> "Capturer's reagent bank: 1 kinds of reagent in 1 stacks."
  - `.reagent take 2589 5` -> "Took ... [Linen Cloth] x5 from the reagent bank."
  - `.reagent deposit` -> "Deposited 5 reagents from 1 stacks."

## Decisions made
- None. Followed existing regression scenario patterns.

## For the other agent
- None.

## Next
- T-071: docs/MAP.md coverage check.

## Map additions
- `tests/regression/scenarios/reagent-bank.json`
