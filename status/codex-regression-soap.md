# Regression harness: complete SOAP and empty bot snapshots

**Agent:** codex
**Branch:** codex/fix-regression-soap
**Task status:** review

## What changed
- Regression runs request complete SOAP replies; interactive preview remains 500 characters.
- Capture baseline checks the always-present nearby_bots array so the supported none profile works. Capture unit tests still verify real last-decision serialization.

## How it was tested
- Python syntax checks and all three live regressions passed during T-011/T-013 validation.
- Independent default RelWithDebInfo worldserver build passes; all 11 baseline unit suites pass; all three regressions pass against that exact binary with the none bot-load profile.

## Next
- Routine tools-only merge authorized by the human. No core source or SQL changes.
