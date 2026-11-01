# SQL Update Audit Report

This document audits all agent-authored SQL migration updates in `sql/updates/` as mandated by **T-053**.
Audit tool: `tools/sql-validator/sql_validator.py` (`tools.dbc.table.DbcTable` backend).
Rules applied: `AGENTS.md` SQL update rules and `docs/ID_RANGES.md`.

> **Policy Note:** Report only. Per `AGENTS.md` and `TASKS.md`, migrations that have already been applied are never edited retroactively. Suggested fixes are documented for future follow-up migrations.

---

## 1. Summary of Checked Files

A total of **23 agent SQL updates** across `sql/updates/characters/`, `sql/updates/master/auth/`, and `sql/updates/world/` were audited.

| # | File | Agent | Subsystem | Validation Status |
|---|---|---|---|---|
| 1 | `sql/updates/characters/2026_10_31_claude_00_heartseed_state.sql` | Claude | Heartseed character state | Clean |
| 2 | `sql/updates/characters/2026_10_31_claude_05_heartseed_fed_weapon.sql` | Claude | Heartseed fed weapons | Clean |
| 3 | `sql/updates/characters/2026_10_31_claude_08_weapon_appearance.sql` | Claude | Weapon transmog/appearance | Clean |
| 4 | `sql/updates/characters/2026_10_31_claude_16_heartseed_account.sql` | Claude | Account-wide seed unlocking | Clean |
| 5 | `sql/updates/master/auth/2026_10_31_claude_02_heartseed_command.sql` | Claude | RBAC for `.heartseed` | Clean |
| 6 | `sql/updates/master/auth/2026_10_31_claude_11_seed_command.sql` | Claude | RBAC for `.seed` alias | Clean |
| 7 | `sql/updates/master/auth/2026_10_31_claude_13_gmtools_commands.sql` | Claude | RBAC for `.checkpoint` commands | Clean |
| 8 | `sql/updates/master/auth/2026_10_31_claude_18_gmtools_t039.sql` | Claude | RBAC for `.bot`, `.teleport`, `.kill` | Clean |
| 9 | `sql/updates/world/2026_10_31_claude_01_heartseed_stage.sql` | Claude | `heartseed_stage_template` | Clean |
| 10 | `sql/updates/world/2026_10_31_claude_03_heartseed_trait.sql` | Claude | `heartseed_trait_template` | Clean |
| 11 | `sql/updates/world/2026_10_31_claude_04_heartseed_essence.sql` | Claude | `heartseed_essence_template` | Clean |
| 12 | `sql/updates/world/2026_10_31_claude_06_heartseed_feed_prompt.sql` | Claude | Broadcast texts & gossip prompts | Clean |
| 13 | `sql/updates/world/2026_10_31_claude_07_twink_item.sql` | Claude | `twink_item` reference table | Clean |
| 14 | `sql/updates/world/2026_10_31_claude_09_heartseed_grant_quest.sql` | Claude | Heartseed granting quest templates | Clean |
| 15 | `sql/updates/world/2026_10_31_claude_10_heartseed_choice.sql` | Claude | Heartseed choice quest & gossip | Clean |
| 16 | `sql/updates/world/2026_10_31_claude_12_gmtools_checkpoint.sql` | Claude | Checkpoint definition tables | Clean |
| 17 | `sql/updates/world/2026_10_31_claude_14_gmtools_twink_candidates.sql` | Claude | Candidate item definitions | Clean |
| 18 | `sql/updates/world/2026_10_31_claude_15_gmtools_twink_checkpoints.sql` | Claude | Twink kit checkpoint associations | Clean |
| 19 | `sql/updates/world/2026_10_31_claude_17_checkpoint_account_seed.sql` | Claude | Account seed checkpoint row | Clean |
| 20 | `sql/updates/world/2026_10_31_claude_19_gmtools_scenario.sql` | Claude | Scenario checkpoint definition | Clean |
| 21 | `sql/updates/world/2026_10_31_codex_00_twink_gear.sql` | Codex | Item template twink stat scaling | Collision with #23 |
| 22 | `sql/updates/world/2026_10_31_codex_01_rare_mount_drops.sql` | Codex | Rare mount loot rates | Clean |
| 23 | `sql/updates/world/2026_10_31_gemini_00_twink_gear.sql` | Gemini | Item template twink stat-buff tuning | Collision with #21 |

---

## 2. Detailed Findings and Recommendations

### Characters Database Updates (`sql/updates/characters/`)

#### 1. `sql/updates/characters/2026_10_31_claude_00_heartseed_state.sql`
- **Tables Touched:** `character_heartseed_state` (DDL `CREATE TABLE IF NOT EXISTS`).
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `00`).
  - Line endings: CRLF.
  - Idempotence: Uses `CREATE TABLE IF NOT EXISTS` with primary key `(guid)`.
  - ID ranges: N/A (DDL schema).
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 2. `sql/updates/characters/2026_10_31_claude_05_heartseed_fed_weapon.sql`
- **Tables Touched:** `character_heartseed_fed_weapon` (DDL `CREATE TABLE IF NOT EXISTS`).
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `05`).
  - Line endings: CRLF.
  - Idempotence: Uses `CREATE TABLE IF NOT EXISTS` with composite primary key `(guid, item_guid)`.
  - ID ranges: N/A.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 3. `sql/updates/characters/2026_10_31_claude_08_weapon_appearance.sql`
- **Tables Touched:** `character_heartseed_weapon_appearance` (DDL `CREATE TABLE IF NOT EXISTS`).
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `08`).
  - Line endings: CRLF.
  - Idempotence: Uses `CREATE TABLE IF NOT EXISTS` with primary key `(guid)`.
  - ID ranges: N/A.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 4. `sql/updates/characters/2026_10_31_claude_16_heartseed_account.sql`
- **Tables Touched:** `account_heartseed_unlock` (DDL `CREATE TABLE IF NOT EXISTS`).
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `16`).
  - Line endings: CRLF.
  - Idempotence: Uses `CREATE TABLE IF NOT EXISTS` with primary key `(account_id)`.
  - ID ranges: N/A.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

---

### Auth Database Updates (`sql/updates/master/auth/`)

#### 5. `sql/updates/master/auth/2026_10_31_claude_02_heartseed_command.sql`
- **Tables Touched:** `rbac_permissions`, `rbac_linked_permissions`.
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `02`).
  - Line endings: CRLF.
  - Idempotence: Follows strict `DELETE FROM ... WHERE id = ...` preceding `INSERT INTO ...`.
  - ID ranges: ID `1030` is within allowed RBAC custom range.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 6. `sql/updates/master/auth/2026_10_31_claude_11_seed_command.sql`
- **Tables Touched:** `rbac_permissions`, `rbac_linked_permissions`.
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `11`).
  - Line endings: CRLF.
  - Idempotence: `DELETE`-then-`INSERT` on ID `1031`.
  - ID ranges: ID `1031` is within custom RBAC range.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 7. `sql/updates/master/auth/2026_10_31_claude_13_gmtools_commands.sql`
- **Tables Touched:** `rbac_permissions`, `rbac_linked_permissions`.
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `13`).
  - Line endings: CRLF.
  - Idempotence: `DELETE`-then-`INSERT` on IDs `1032` through `1035`.
  - ID ranges: Custom RBAC command IDs.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 8. `sql/updates/master/auth/2026_10_31_claude_18_gmtools_t039.sql`
- **Tables Touched:** `rbac_permissions`, `rbac_linked_permissions`.
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `18`).
  - Line endings: CRLF.
  - Idempotence: `DELETE`-then-`INSERT` on IDs `1036` through `1040`.
  - ID ranges: Valid RBAC command IDs.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

---

### World Database Updates (`sql/updates/world/`)

#### 9. `sql/updates/world/2026_10_31_claude_01_heartseed_stage.sql`
- **Tables Touched:** `heartseed_stage_template` (DDL + DML).
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `01`).
  - Line endings: CRLF.
  - Idempotence: `CREATE TABLE IF NOT EXISTS`, followed by `DELETE`-then-`INSERT` for stages 1 to 6.
  - ID ranges: Clean.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 10. `sql/updates/world/2026_10_31_claude_03_heartseed_trait.sql`
- **Tables Touched:** `heartseed_trait_template` (DDL + DML).
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `03`).
  - Line endings: CRLF.
  - Idempotence: `CREATE TABLE IF NOT EXISTS`, followed by `DELETE`-then-`INSERT` for trait IDs `1..24`.
  - References: Proc spells reference valid DBC spell IDs.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 11. `sql/updates/world/2026_10_31_claude_04_heartseed_essence.sql`
- **Tables Touched:** `heartseed_essence_template` (DDL + DML).
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `04`).
  - Line endings: CRLF.
  - Idempotence: `CREATE TABLE IF NOT EXISTS`, followed by `DELETE`-then-`INSERT`.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 12. `sql/updates/world/2026_10_31_claude_06_heartseed_feed_prompt.sql`
- **Tables Touched:** `broadcast_text`.
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `06`).
  - Line endings: CRLF.
  - Idempotence: `DELETE`-then-`INSERT` for ID `90001`.
  - ID ranges: ID `90001` within Claude's broadcast_text allocation (`90000..90999`).
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 13. `sql/updates/world/2026_10_31_claude_07_twink_item.sql`
- **Tables Touched:** `twink_item` (DDL + DML).
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `07`).
  - Line endings: CRLF.
  - Idempotence: `CREATE TABLE IF NOT EXISTS`, followed by scoped `DELETE`-then-`INSERT` for twink item entries.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 14. `sql/updates/world/2026_10_31_claude_09_heartseed_grant_quest.sql`
- **Tables Touched:** `quest_template`, `quest_template_addon`, `creature_queststarter`.
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `09`).
  - Line endings: CRLF.
  - Idempotence: Clean `DELETE FROM ... WHERE ID IN (...)` before `INSERT`.
  - ID ranges: Quest IDs `90001..90005` in Claude's range.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 15. `sql/updates/world/2026_10_31_claude_10_heartseed_choice.sql`
- **Tables Touched:** `gossip_menu`, `gossip_menu_option`, `npc_text`.
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `10`).
  - Line endings: CRLF.
  - Idempotence: Scoped `DELETE`-then-`INSERT`.
  - ID ranges: Gossip IDs `90010..90015` in Claude's range.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 16. `sql/updates/world/2026_10_31_claude_12_gmtools_checkpoint.sql`
- **Tables Touched:** `gmtools_checkpoint`, `gmtools_checkpoint_item`, `gmtools_checkpoint_spell`.
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `12`).
  - Line endings: CRLF.
  - Idempotence: `CREATE TABLE IF NOT EXISTS` with `DELETE`-then-`INSERT` on primary key strings.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 17. `sql/updates/world/2026_10_31_claude_14_gmtools_twink_candidates.sql`
- **Tables Touched:** `gmtools_twink_candidate` (DDL + DML).
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `14`).
  - Line endings: CRLF.
  - Idempotence: `CREATE TABLE IF NOT EXISTS`, followed by scoped `DELETE`-then-`INSERT`.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 18. `sql/updates/world/2026_10_31_claude_15_gmtools_twink_checkpoints.sql`
- **Tables Touched:** `gmtools_checkpoint`, `gmtools_checkpoint_item`.
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `15`).
  - Line endings: CRLF.
  - Idempotence: `DELETE`-then-`INSERT` on checkpoint entries.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 19. `sql/updates/world/2026_10_31_claude_17_checkpoint_account_seed.sql`
- **Tables Touched:** `gmtools_checkpoint`.
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `17`).
  - Line endings: CRLF.
  - Idempotence: `DELETE`-then-`INSERT` on `account-seed`.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 20. `sql/updates/world/2026_10_31_claude_19_gmtools_scenario.sql`
- **Tables Touched:** `gmtools_checkpoint`.
- **Audit Checks:**
  - Filename matches format: Yes (`claude`, seq `19`).
  - Line endings: CRLF.
  - Idempotence: `DELETE`-then-`INSERT` on `scenario-bot-level-10`.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 21. `sql/updates/world/2026_10_31_codex_00_twink_gear.sql`
- **Tables Touched:** `item_template`, `hotfix_data`.
- **Audit Checks:**
  - Filename matches format: Yes (`codex`, seq `00`).
  - Line endings: CRLF.
  - Idempotence: 314 scoped `UPDATE item_template ... WHERE entry = <id>` and corresponding `DELETE`-then-`INSERT` on `hotfix_data`.
  - Individual validation: Clean (0 errors).
  - Collective validation: **Collision on all 314 item entries** with `2026_10_31_gemini_00_twink_gear.sql`.
- **Finding:** Cross-agent modification collision with Gemini's twink tuning update.
- **Suggested Fix:**
  See Section 3 below. Both files perform conflicting `UPDATE` operations on the exact same 314 item IDs. A reconciling migration should be scheduled to consolidate into a single source of truth for twink item stat baselines.

#### 22. `sql/updates/world/2026_10_31_codex_01_rare_mount_drops.sql`
- **Tables Touched:** `creature_loot_template`, `gameobject_loot_template`, `item_loot_template`, `reference_loot_template`, `spell_loot_template`.
- **Audit Checks:**
  - Filename matches format: Yes (`codex`, seq `01`).
  - Line endings: CRLF.
  - Idempotence: Fully scoped `UPDATE ... WHERE Entry = ... AND Item = ...`.
  - References: Valid items and creature entries.
- **Finding:** Clean.
- **Suggested Fix:** None needed.

#### 23. `sql/updates/world/2026_10_31_gemini_00_twink_gear.sql`
- **Tables Touched:** `item_template`, `hotfix_data`.
- **Audit Checks:**
  - Filename matches format: Yes (`gemini`, seq `00`).
  - Line endings: CRLF.
  - Idempotence: 314 scoped `UPDATE item_template ... WHERE entry = <id>` and `DELETE`-then-`INSERT` on `hotfix_data`.
  - Individual validation: Clean (0 errors).
  - Collective validation: **Collision on all 314 item entries** with `2026_10_31_codex_00_twink_gear.sql`.
- **Finding:** Cross-agent modification collision with Codex's twink gear update.
- **Suggested Fix:** See Section 3 below.

---

## 3. Cross-Agent Collision Analysis: Twink Gear Updates

### Description
In T-045, Codex created `2026_10_31_codex_00_twink_gear.sql` to apply stat adjustments across 314 level-19 twink items.
Subsequently in T-050, Gemini generated `2026_10_31_gemini_00_twink_gear.sql` with revised stat-buff values across the exact same 314 item entries.

When applying migrations sequentially:
1. Codex's update sets the base twink stats.
2. Gemini's update overwrites the stats for all 314 items.
Because both updates target the exact same table rows with conflicting values, the validator flags 314 `id_collision` errors:
```
sql/updates/world/2026_10_31_gemini_00_twink_gear.sql:<line>: [id_collision] Cross-agent modification collision on `item_template`.entry=<entry> between agent 'gemini' (sql/updates/world/2026_10_31_gemini_00_twink_gear.sql:<line>) and agent 'codex' (sql/updates/world/2026_10_31_codex_00_twink_gear.sql:<line>)
```

### Recommendation
1. **Never edit existing applied files:** Both updates have been committed to `origin/playerbot` and applied to active test databases.
2. **Clarification for downstream agents:** `gemini_00_twink_gear.sql` represents the intended T-050 tuned values for twink gameplay.
3. **Consolidation Migration:** When next modifying twink items, a single new migration should supersede both prior updates with a clean note in `status/` acknowledging the consolidation.
