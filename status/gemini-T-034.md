# T-034: SQL update validator (`tools/sql-validator/`)

**Agent:** gemini
**Branch:** gemini/T-034-sql-validator
**Task status:** review

## What changed
- **`tools/sql-validator/sql_validator.py`:** dependency-free Python 3.9+ validator with a generic SQL
  lexer/statement parser and idempotence linter separated from the WoW / UltimateMoP adapter:
  - **Filename & agent (`filename`):** validates `YYYY_MM_DD_<agent>_<NN>_<description>.sql` (valid date,
    agent in `claude`, `codex`, `gemini`, numeric sequence `NN`, lowercase slug) and detects duplicate
    `(YYYY_MM_DD, agent, NN)` sequences across files. Supports `--legacy-before YYYY-MM-DD` for scanning
    directories that also hold pre-fork upstream SQL files.
  - **Line endings (`crlf`):** checks the Git index (`git ls-files --eol -- <paths>`) for tracked files and
    flags any file whose index EOL (`i/`) is not `lf` (`i/crlf`, `i/mixed`), skipping untracked files so
    Windows `core.autocrlf=true` checkouts validate cleanly without extra flags.
  - **Idempotence (`idempotence`):** rejects bare `INSERT INTO` without `INSERT IGNORE`, `REPLACE INTO`,
    `ON DUPLICATE KEY UPDATE`, `SELECT ... WHERE NOT EXISTS`, or a preceding `DELETE FROM` on the same
    table; verifies that preceding `DELETE FROM ... WHERE` predicates (`=`, `IN (...)`, `BETWEEN ... AND ...`)
    cover every inserted primary key; rejects bare `CREATE TABLE` without `IF NOT EXISTS` (unless `TEMPORARY`
    or preceded by `DROP TABLE IF EXISTS`); rejects unscoped `UPDATE` statements without `WHERE` (except on
    `TEMPORARY` tables) and relative self-updates (`SET col = col + ...`); rejects client/session directives
    (`USE`, `SOURCE`, `\!`, `CREATE/DROP DATABASE`, `/*! ... */`).
  - **ID ranges & collisions (`id_range`, `id_collision`):** dynamically parses `docs/ID_RANGES.md` and
    validates custom IDs (`quest_template`, `quest_offer_reward`, `quest_request_items`,
    `quest_template_addon`, `creature_template`, `gameobject_template`, `item_template`, `spell_dbc`,
    `spellmisc_dbc`, `spelleffect_dbc`, `gossip_menu`, `gossip_menu_option`, `npc_text`, `rbac_permissions`),
    flags unallocated `Next system` IDs and cross-system block mismatches, and detects primary-key and
    custom-ID collisions across agents' files (`INSERT` and `UPDATE`).
  - **Schema & world DB references (`schema`, `reference`):** queries `information_schema.COLUMNS` and the
    named `--world-db` (plus optional `--extra-db` for `characters`/`auth` and `--dbc-dir` for `Spell.dbc`)
    to check column names, `INSERT` value tuple arity, and foreign references to quest givers/enders,
    quests, items, creatures, gameobjects, and spells (including intra-corpus `CREATE TABLE` and `INSERT`
    definitions).
- **`tools/sql-validator/test_sql_validator.py`:** 6 unit tests covering filename/agent validation, CRLF
  detection, idempotence rules, `docs/ID_RANGES.md` blocks and cross-agent collisions, WDBC spell loading
  and world reference/schema checks, and the `2026_10_31_*` agent SQL corpus.
- **`tools/sql-validator/README.md`:** purpose, checks, CLI flags, environment variables, and usage.
- **Gemini environment setup:** initialized `auth_gemini`, `characters_gemini`, `world_gemini`, and
  `playerbots_gemini` from the v1.0.4 release dumps via `tools/devenv/setup.py --agent gemini` and applied
  all 16 merged `2026_10_31_*` updates to `auth_gemini`, `characters_gemini`, and `world_gemini`.

## How it was tested
- Unit tests: `python -m unittest discover -s tools/sql-validator -v` (6 tests, all pass in 0.28 s).
- Corpus run against live `world_gemini`, `characters_gemini`, `auth_gemini`, and `gamedata/dbc/Spell.dbc`:
  - **16 agent files (`sql/updates/**/2026_10_31_*.sql`):** **0 issues** (`Checked 16 file(s): 0 issue(s) found`).
  - **All 170 `sql/updates/**/2026_*.sql` files:**
    - All 170 tracked files pass the `git ls-files --eol` index check (`i/lf`).
    - Without `--legacy-before 2026-10-30`: the 154 pre-fork upstream files (`2026_01_19_*` to `2026_10_26_*`)
      are flagged on `[filename]`.
    - With `--legacy-before 2026-10-30`: 168 of 170 files pass cleanly; 2 pre-fork upstream
      files report 6 genuine idempotence issues (`2026_09_08_00_world.sql` lines 88–94 inserting 5
      `spell_script_names` rows not covered by earlier `DELETE`s, and
      `2026_10_06_67_world_trinity_string_bg_reward.sql` line 13 bare `INSERT INTO trinity_string`).

## Decisions made
- Client spells live in `Spell.dbc` (94,842 rows) while server-side custom spells live in `world.spell_dbc`
  (479 base rows), so `--dbc-dir` loads `Spell.dbc` and unions its IDs with `world.spell_dbc` and any
  `spell_dbc` rows defined in the SQL corpus being validated.
- Negative `spell` values in `npc_trainer` reference shared `npc_trainer.entry` templates rather than
  spell IDs (`ObjectMgr::LoadTrainerSpell`), and negative `mincountOrRef` values in `*_loot_template`
  reference `reference_loot_template`, so only positive IDs are checked against `Spell.dbc` / `item_template`.

## For the other agents
- Run `python tools/sql-validator/sql_validator.py <your-sql-files> --world-db world_<agent> --dbc-dir I:/UltimateMoP/gamedata/dbc`
  before opening a PR with new SQL updates.
- Note that `core.autocrlf=true` is set in the repo config without a `.gitattributes` rule for `*.sql`, so
  checked-out working-tree `.sql` files have `w/crlf` on disk even when committed as `i/lf` in the index.

## Next
- T-030: Shared DBC reader library (`tools/dbc/`) and content search MCP (`tools/content-search/`).

## Map additions
- `tools/sql-validator/`: SQL update linter (filename, CRLF, idempotence, ID ranges, collisions, schema and world references).
