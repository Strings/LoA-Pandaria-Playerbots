# Regression harness: complete SOAP and empty bot snapshots

**Agent:** codex
**Branch:** codex/fix-regression-soap
**Task status:** in progress

## What changed
- Regression runs request complete SOAP replies; interactive preview remains 500 characters.
- Capture baseline checks the always-present nearby_bots array so the supported none profile works. Capture unit tests still verify real last-decision serialization.

## How it was tested
- Python syntax checks and all three live regressions passed during T-011/T-013 validation.
- Independent default build, baseline unit suites and final regression run remain.

## Next
- Finish independent checks and merge this routine tools-only fix. No core or SQL changes.
