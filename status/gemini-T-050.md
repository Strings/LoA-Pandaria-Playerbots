# T-050: Twink buff generator

**Agent:** gemini
**Branch:** gemini/T-050-twink-tuning
**Task status:** review

## What changed
- Added `tools/twink-tuning/` (`tuning_math.py`, `twink_tuning.py`, `v104_baseline.json`, `test_twink_tuning.py`, `README.md`) to regenerate twink stat-buff SQL migrations from live read-only `item_template` data and MoP 5.4.8 DBC scaling curves (`ScalingStatDistribution.dbc`, `ScalingStatValues.dbc`, `RandPropPoints.dbc`, `gtItemSocketCostPerLevel.dbc`, `ItemDamage*.dbc`).
- Supports `--target-pct`, `--weights`, and `--brackets` CLI arguments; defaults (`7%`, `docs/twinks.md` weights and 14 brackets) reproduce all 314 `UPDATE item_template` and `hotfix_data` statements identically to `sql/updates/world/2026_10_31_codex_00_twink_gear.sql` (`0` diff lines).
- Generated the successor migration `sql/updates/world/2026_10_31_gemini_00_twink_gear.sql` (leaving Codex's original file untouched).
- Included `v104_baseline.json` with the stock v1.0.4 `ItemLevel` and `stat_*` values of the 314 modified items so re-running the generator read-only against a database that already has the twink migration applied produces identical results.

## How it was tested
- Ran unit tests (`python -m unittest discover -s tools/twink-tuning -v`): 6/6 tests pass (stat weights parsing, bracket stat budgets and `MaxLevel` cap, `ceil` rounding, `stat_unk1_N` allocation rebasing with socket penalties, comparator selection, and weapon `ItemLevel` floor).
- Ran `python tools/twink-tuning/twink_tuning.py` against `world_gemini` and verified `git diff --no-index sql/updates/world/2026_10_31_codex_00_twink_gear.sql sql/updates/world/2026_10_31_gemini_00_twink_gear.sql` is empty (`0` differences).
- Validated `sql/updates/world/2026_10_31_gemini_00_twink_gear.sql` with `tools/sql-validator/sql_validator.py --world-db world_gemini --dbc-dir I:\UltimateMoP\gamedata\dbc`: `0 issue(s) found`.
- Applied `sql/updates/world/2026_10_31_gemini_00_twink_gear.sql` to `world_gemini` twice and verified `item_template` and `hotfix_data` (`315` rows) snapshots are 100% identical after both runs, and confirmed re-running `twink_tuning.py` against the patched `world_gemini` still reproduces the exact same file.

## Decisions made
- Bundled a lightweight `v104_baseline.json` (314 rows) so `twink_tuning.py` stays 100% read-only against the database while remaining idempotent even when run against a database where the twink gear buffs have already been applied.

## For the other agent
- `sql/updates/world/2026_10_31_gemini_00_twink_gear.sql` is the generated successor to `sql/updates/world/2026_10_31_codex_00_twink_gear.sql`. Once merged, I will apply it to the main `world` database per `AGENTS.md`.

## Next
- Proceed to `T-033` (`tools/crash-triage/`).

## Map additions
- `tools/twink-tuning/`: Twink stat-buff SQL generator and scaling math (`twink_tuning.py`, `tuning_math.py`).
