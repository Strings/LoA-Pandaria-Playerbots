# T-071: docs/MAP.md coverage check

**Agent:** gemini
**Branch:** gemini/T-071-map-coverage-check
**Task status:** review

## What changed
- Audited `docs/MAP.md` against the repository filesystem:
  - Corrected directory paths in the Core table for `Entities/Unit/MountedSpeed.cpp`, `World/RespecConfig.cpp`, `World/CurrencyLimits.h`, and `src/common/Utilities/RandomSeedState.h`.
  - Added missing top-level paths: `docs/`, `status/`, `lua_scripts/`, `doc/`.
  - Added missing tooling paths: `tools/crash-triage/`, `tools/gmtools/`, `tools/stack-sizes/`, `tools/map-check/`.
  - Added missing test suite paths: `tests/framework/`, `tests/capture/`, `tests/common/`, `tests/gmtools/`, `tests/solo_config/`, `tests/regression/`.
  - Added dedicated `## Documentation (docs/)` table mapping `DESIGN.md`, `ID_RANGES.md`, `MAP.md`, `PLAYTEST.md`, `TUNING.md`, `twinks.md`, and `agents/`.
- Added automated coverage verifier `tools/map-check/check_map.py` to ensure every path referenced in `docs/MAP.md` exists on disk and every major subsystem in `src/server/scripts/Custom/`, `tools/`, `tests/`, `addons/`, and `docs/` is documented.
- Added task row for T-071 in `TASKS.md`.

## How it was tested
- Ran `python tools/map-check/check_map.py`:
  ```
  Verifying 105 paths from docs/MAP.md...
  Results:
    MAP.md paths missing from disk: 0
    Major paths missing from MAP.md: 0
  OK: docs/MAP.md coverage check passed with 0 missing paths.
  ```

## Decisions made
- Created `tools/map-check/check_map.py` so agents and CI can continuously assert that `docs/MAP.md` remains complete and free of stale or broken paths.

## For the other agent
- `python tools/map-check/check_map.py` can be run anytime to verify that newly created subsystems are represented in `docs/MAP.md`.

## Next
- None. Flash queue complete.

## Map additions
- `tools/map-check/`
