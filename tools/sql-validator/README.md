# SQL Validator (`tools/sql-validator/`)

Dependency-free Python 3.9+ validator for UltimateMoP SQL update files (`T-034`).
Separates the generic SQL statement parser and idempotence linter from the WoW / UltimateMoP
adapter (`docs/ID_RANGES.md` blocks, cross-agent collision detection, MySQL schema and foreign-key
reference checks, and `Spell.dbc` verification).

## Checks Performed

1. **Filename pattern & agent (`filename`):**
   - Enforces `YYYY_MM_DD_<agent>_<NN>_<description>.sql` with a valid calendar date, an agent in
     `claude`, `codex`, `gemini` (`--agents`), numeric sequence `NN`, and lowercase slug.
   - Detects duplicate `(YYYY_MM_DD, agent, NN)` sequence numbers across files.
   - Use `--legacy-before YYYY-MM-DD` when scanning directories that also include pre-fork upstream
     update files.
2. **Line endings (`crlf`):**
   - Rejects files containing `CRLF` (`\r\n`) or bare `CR` (`\r`) bytes on disk.
   - Use `--ignore-crlf` when inspecting a Windows checkout where `core.autocrlf=true` converted LF
     blobs on checkout.
3. **Idempotence (`idempotence`):**
   - Rejects bare `INSERT INTO` without `INSERT IGNORE`, `REPLACE INTO`, `ON DUPLICATE KEY UPDATE`,
     `SELECT ... WHERE NOT EXISTS`, or a preceding `DELETE FROM` on the same table.
   - Verifies that when a preceding `DELETE FROM <table> WHERE ...` is used, every inserted primary
     key is covered by the `DELETE` predicate (`=`, `IN (...)`, `BETWEEN ... AND ...`).
   - Rejects bare `CREATE TABLE` without `IF NOT EXISTS` (unless `TEMPORARY` or preceded by
     `DROP TABLE IF EXISTS`).
   - Rejects unscoped `UPDATE` statements without `WHERE` (except on `TEMPORARY` tables) and
     non-idempotent relative self-updates (`SET col = col + ...`).
   - Rejects session/client directives (`USE`, `SOURCE`, `\!`, `CREATE/DROP DATABASE`, `/*! ... */`).
4. **Reserved ID ranges & cross-agent collisions (`id_range`, `id_collision`):**
   - Parses `docs/ID_RANGES.md` dynamically and checks custom quests, creatures, gameobjects, items,
     spells (`spell_dbc`, `spellmisc_dbc`, `spelleffect_dbc`), gossip (`gossip_menu`, `npc_text`),
     and RBAC permissions (`rbac_permissions`).
   - Rejects unallocated `Next system` IDs, cross-system block mismatches (e.g. a Heartseed file
     using a GM tools ID), and non-reserved stock-range inserts in agent files.
   - Detects primary-key and custom-ID collisions across agents' files (`INSERT` and `UPDATE`).
5. **Schema & world reference validation (`schema`, `reference`):**
   - When `--world-db` (or `MOP_WORLD_DB`) is provided, queries `information_schema.COLUMNS` and the
     named world database (plus optional `--extra-db` for `characters`/`auth` and `--dbc-dir` for
     `Spell.dbc`) to verify:
     - Table column names and `INSERT` value tuple arity.
     - Quest givers and enders (`creature_queststarter`, `creature_questender`,
       `gameobject_queststarter`, `gameobject_questender`).
     - Quests (`quest_template`, `quest_offer_reward`, `quest_request_items`,
       `quest_template_addon`).
     - Items (`item_template`, `twink_item`, `gmtools_kit_item`, `heartseed_essence_source`,
       `hotfix_data`, `npc_vendor`, `achievement_reward`, and `*_loot_template`).
     - Creatures & gameobjects (`creature`, `creature_template_*`, `creature_equip_template`,
       `creature_text`, `vehicle_template_accessory`, `npc_trainer`, `npc_spellclick_spells`,
       `smart_scripts`).
     - Spells (`heartseed_trait`, `heartseed_essence_template`, `spelleffect_dbc`,
       `spell_script_names`, `spell_area`, `spell_custom_attribute`, `spell_linked_spell`,
       `playercreateinfo_action`) against both `world.spell_dbc` and `Spell.dbc`.

## Usage

```powershell
# Static + ID range + collision check on agent SQL updates
python tools/sql-validator/sql_validator.py "sql/updates/**/2026_10_31_*.sql"

# Full check including live world DB references, schema columns, and Spell.dbc
$env:MOP_DB_PASSWORD = "ultimatemop"
python tools/sql-validator/sql_validator.py "sql/updates/**/2026_10_31_*.sql" `
  --world-db world_gemini `
  --extra-db characters_gemini `
  --extra-db auth_gemini `
  --dbc-dir I:/UltimateMoP/gamedata/dbc `
  --mysql-bin "C:/Program Files/MySQL/MySQL Server 8.0/bin"
```

## Configuration & Environment Variables

| Flag | Environment Variable | Default | Purpose |
|---|---|---|---|
| `--world-db` | `MOP_WORLD_DB` | `""` | World DB for foreign-key and schema checks |
| `--extra-db` | — | `[]` | Extra DBs (`characters_*`, `auth_*`) for schema checks |
| `--dbc-dir` | `MOP_DBC_DIR` | `""` | Path to `dbc/` folder containing `Spell.dbc` |
| `--mysql-bin` | `MOP_MYSQL_BIN` | `""` | Directory containing `mysql` client binary |
| `--db-host` | `MOP_DB_HOST` | `127.0.0.1` | MySQL host |
| `--db-port` | `MOP_DB_PORT` | `3306` | MySQL port |
| `--db-user` | `MOP_DB_USER` | `ultimatemop` | MySQL user |
| `--db-password-env` | — | `MOP_DB_PASSWORD` | Name of env var holding MySQL password |

## Unit Tests

```powershell
python -m unittest discover -s tools/sql-validator -v
```
