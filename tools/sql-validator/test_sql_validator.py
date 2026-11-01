from datetime import date
from pathlib import Path
import struct
import tempfile
import unittest

import sql_validator as v


class SqlValidatorTests(unittest.TestCase):
    def setUp(self):
        self.temp_root = v.ROOT / 'build/test-temporary'
        self.temp_root.mkdir(parents=True, exist_ok=True)
        self.blocks = v.parse_id_ranges((v.ROOT / 'docs/ID_RANGES.md').read_text(encoding='utf-8'))

    def _write_sql(self, folder: Path, name: str, content: bytes) -> Path:
        path = folder / name
        path.write_bytes(content)
        return path

    def test_filename_and_agent_validation(self):
        with tempfile.TemporaryDirectory(dir=self.temp_root) as tmp:
            root = Path(tmp)
            good = self._write_sql(root, '2026_10_31_gemini_00_valid_table.sql',
                                   b'CREATE TABLE IF NOT EXISTS `heartseed_test` (`id` INT PRIMARY KEY);\n')
            bad_agent = self._write_sql(root, '2026_10_31_otherbot_00_bad_agent.sql',
                                        b'CREATE TABLE IF NOT EXISTS `heartseed_test` (`id` INT PRIMARY KEY);\n')
            bad_date = self._write_sql(root, '2026_02_30_gemini_01_bad_date.sql',
                                       b'CREATE TABLE IF NOT EXISTS `heartseed_test` (`id` INT PRIMARY KEY);\n')
            bad_pattern = self._write_sql(root, '2026_10_31_00_missing_agent.sql',
                                          b'CREATE TABLE IF NOT EXISTS `heartseed_test` (`id` INT PRIMARY KEY);\n')
            dup_seq = self._write_sql(root, '2026_10_31_gemini_00_duplicate_seq.sql',
                                      b'CREATE TABLE IF NOT EXISTS `heartseed_test2` (`id` INT PRIMARY KEY);\n')

            self.assertEqual(v.validate_files([good]), [])

            issues = v.validate_files([bad_agent, bad_date, bad_pattern])
            categories = [i.category for i in issues]
            self.assertEqual(categories, ['filename', 'filename', 'filename'])
            self.assertIn("Unknown agent 'otherbot'", issues[0].message)
            self.assertIn("('claude', 'codex', 'gemini')", issues[0].message)

            # Duplicate (date, agent, NN) across two files
            seq_issues = v.validate_files([good, dup_seq])
            self.assertEqual(len(seq_issues), 1)
            self.assertEqual(seq_issues[0].category, 'filename')
            self.assertIn('Duplicate agent sequence number', seq_issues[0].message)

            # --legacy-before allows pre-fork upstream filenames on or before cutoff
            legacy = self._write_sql(root, '2026_10_26_00_upstream_update.sql',
                                     b'DELETE FROM `npc_text` WHERE `ID` = 1;\nINSERT INTO `npc_text` (`ID`) VALUES (1);\n')
            self.assertEqual(len(v.validate_files([legacy])), 1)
            self.assertEqual(v.validate_files([legacy], allow_legacy_before=date(2026, 10, 30)), [])

    def test_crlf_line_endings_detected(self):
        # Verify git ls-files --eol parser flags i/crlf and i/mixed, passes i/lf, and skips untracked
        eol_output = (
            "i/lf    w/crlf  attr/                 \tsql/updates/world/2026_10_31_gemini_00_clean.sql\n"
            "i/crlf  w/crlf  attr/                 \tsql/updates/world/2026_10_31_gemini_01_bad_crlf.sql\n"
            "i/mixed w/mixed attr/                 \tsql/updates/world/2026_10_31_gemini_02_bad_mixed.sql\n"
            "i/      w/      attr/                 \tsql/updates/world/2026_10_31_gemini_03_untracked.sql\n"
        )
        issues = v.parse_git_eol_output(eol_output)
        self.assertEqual(len(issues), 2)
        self.assertEqual(issues[0].file, 'sql/updates/world/2026_10_31_gemini_01_bad_crlf.sql')
        self.assertIn("i/crlf", issues[0].message)
        self.assertEqual(issues[1].file, 'sql/updates/world/2026_10_31_gemini_02_bad_mixed.sql')
        self.assertIn("i/mixed", issues[1].message)

    def test_missing_password_env_raises_clear_error(self):
        import os
        from unittest.mock import patch
        agent_file = str(next((v.ROOT / 'sql/updates').rglob('2026_10_31_*.sql')))
        with patch.dict(os.environ, {}, clear=False):
            os.environ.pop('MOP_DB_PASSWORD', None)
            with self.assertRaises(ValueError) as ctx:
                v.main([agent_file, '--world-db', 'world_gemini'])
            self.assertIn('set MOP_DB_PASSWORD (or pass --db-password-env)', str(ctx.exception))

    def test_idempotence_rules(self):
        with tempfile.TemporaryDirectory(dir=self.temp_root) as tmp:
            root = Path(tmp)
            # 1. Bare INSERT, bare CREATE TABLE, unscoped UPDATE, relative self-update, client directive
            bad_sql = (
                "CREATE TABLE `heartseed_bad` (`id` INT PRIMARY KEY);\n"
                "INSERT INTO `heartseed_stage` (`stage`, `min_level`, `name`) VALUES (1, 10, 'Sprout');\n"
                "DELETE FROM `npc_text` WHERE `ID` IN (90001, 90002);\n"
                "INSERT INTO `npc_text` (`ID`, `text0_0`) VALUES (90001, 'ok'), (90003, 'uncovered');\n"
                "UPDATE `item_template` SET `ItemLevel` = 50;\n"
                "UPDATE `item_template` SET `stat_value1` = `stat_value1` + 5 WHERE `entry` = 1482;\n"
                "USE `world`;\n"
            ).encode('utf-8')
            bad_path = self._write_sql(root, '2026_10_31_gemini_00_non_idempotent.sql', bad_sql)
            issues = v.validate_files([bad_path])
            msgs = [i.message for i in issues if i.category == 'idempotence']
            self.assertEqual(len(msgs), 6)
            self.assertTrue(any('Bare CREATE TABLE `heartseed_bad`' in m for m in msgs))
            self.assertTrue(any('Bare INSERT INTO `heartseed_stage`' in m for m in msgs))
            self.assertTrue(any('not covered by preceding DELETE: 90003' in m for m in msgs))
            self.assertTrue(any('Unscoped UPDATE on `item_template`' in m for m in msgs))
            self.assertTrue(any('Relative self-update on `item_template`' in m for m in msgs))
            self.assertTrue(any('Disallowed database/session statement: USE' in m for m in msgs))

            # 2. Valid idempotent constructs pass
            good_sql = (
                "CREATE TABLE IF NOT EXISTS `heartseed_good` (`id` INT PRIMARY KEY);\n"
                "DELETE FROM `npc_text` WHERE `ID` BETWEEN 90001 AND 90003;\n"
                "INSERT INTO `npc_text` (`ID`, `text0_0`) VALUES (90001, 'a'), (90002, 'b');\n"
                "INSERT INTO `rbac_permissions` (`id`, `name`) VALUES (90005, 'Command: hs') "
                "ON DUPLICATE KEY UPDATE `name` = VALUES(`name`);\n"
                "UPDATE `item_template` SET `ItemLevel` = 38 WHERE `entry` = 1482;\n"
            ).encode('utf-8')
            good_path = self._write_sql(root, '2026_10_31_gemini_01_idempotent.sql', good_sql)
            self.assertEqual(v.validate_files([good_path]), [])

    def test_id_ranges_and_cross_agent_collisions(self):
        with tempfile.TemporaryDirectory(dir=self.temp_root) as tmp:
            root = Path(tmp)
            # Heartseed file using GM tools quest ID (91000), unallocated Next system spell (314000), and stock quest (500)
            bad_range_sql = (
                "DELETE FROM `quest_template` WHERE `ID` IN (500, 91000);\n"
                "INSERT INTO `quest_template` (`ID`) VALUES (500), (91000);\n"
                "DELETE FROM `spell_dbc` WHERE `Id` = 314000;\n"
                "INSERT INTO `spell_dbc` (`Id`) VALUES (314000);\n"
            ).encode('utf-8')
            f_range = self._write_sql(root, '2026_10_31_claude_00_heartseed_bad_ids.sql', bad_range_sql)
            range_issues = v.validate_files([f_range])
            self.assertEqual([i.category for i in range_issues], ['id_range', 'id_range', 'id_range'])
            self.assertIn('below custom floor 90000', range_issues[0].message)
            self.assertIn("belongs to 'GM tools (T-039, T-048)', but file belongs to 'Heartseed'", range_issues[1].message)
            self.assertIn('unallocated Next system block', range_issues[2].message)

            # Cross-agent collision on npc_text 90010 and item_template entry 1482
            f_claude = self._write_sql(
                root, '2026_10_31_claude_01_heartseed_text.sql',
                (
                    "DELETE FROM `npc_text` WHERE `ID` = 90010;\n"
                    "INSERT INTO `npc_text` (`ID`, `text0_0`) VALUES (90010, 'Claude text');\n"
                    "UPDATE `item_template` SET `ItemLevel` = 40 WHERE `entry` = 1482;\n"
                ).encode('utf-8')
            )
            f_codex = self._write_sql(
                root, '2026_10_31_codex_01_heartseed_text_dup.sql',
                (
                    "DELETE FROM `npc_text` WHERE `ID` = 90010;\n"
                    "INSERT INTO `npc_text` (`ID`, `text0_0`) VALUES (90010, 'Codex text');\n"
                    "UPDATE `item_template` SET `ItemLevel` = 42 WHERE `entry` = 1482;\n"
                ).encode('utf-8')
            )
            collision_issues = v.validate_files([f_claude, f_codex])
            col_msgs = [i.message for i in collision_issues if i.category == 'id_collision']
            self.assertTrue(any('`npc_text`' in m and '90010' in m for m in col_msgs))
            self.assertTrue(any('Cross-agent modification collision on `item_template`.entry=1482' in m for m in col_msgs))

    def test_schema_and_world_references_and_wdbc_loader(self):
        with tempfile.TemporaryDirectory(dir=self.temp_root) as tmp:
            root = Path(tmp)
            # Build a minimal valid WDBC file with spells 9613 and 26415
            dbc_path = root / 'Spell.dbc'
            records = struct.pack('<2I', 9613, 0) + struct.pack('<2I', 26415, 0)
            header = struct.pack('<4s4I', b'WDBC', 2, 2, 8, 1)
            dbc_path.write_bytes(header + records + b'\x00')
            loaded_spells = v.load_wdbc_first_uint32_ids(dbc_path)
            self.assertEqual(loaded_spells, {9613, 26415})

            catalog = v.WorldReferenceCatalog(
                quests={184},
                creatures={42309},
                gameobjects={1000},
                items={1482, 21841},
                spells=loaded_spells,
                table_columns={
                    'creature_queststarter': {'id', 'quest'},
                    'twink_item': {'entry', 'bracket', 'comment'},
                    'heartseed_essence_template': {'id', 'name', 'proc_spell_id'},
                    'creature': {'guid', 'id'},
                },
            )

            bad_refs = self._write_sql(
                root, '2026_10_31_gemini_00_bad_refs.sql',
                (
                    "DELETE FROM `creature_queststarter` WHERE `id` = 999999 AND `quest` = 88888;\n"
                    "INSERT INTO `creature_queststarter` (`id`, `quest`) VALUES (999999, 88888);\n"
                    "DELETE FROM `twink_item` WHERE `entry` = 777777;\n"
                    "INSERT INTO `twink_item` (`entry`, `bracket`, `comment`) VALUES (777777, 19, 'Missing item');\n"
                    "DELETE FROM `heartseed_essence_template` WHERE `id` = 1;\n"
                    "INSERT INTO `heartseed_essence_template` (`id`, `name`, `proc_spell_id`) VALUES (1, 'Bad', 555555);\n"
                    "DELETE FROM `creature` WHERE `guid` = 1;\n"
                    "INSERT INTO `creature` (`guid`, `id`, `nonexistent_col`) VALUES (1, 666666, 0);\n"
                ).encode('utf-8')
            )
            issues = v.validate_files([bad_refs], catalog=catalog)
            ref_msgs = [i.message for i in issues if i.category == 'reference']
            schema_msgs = [i.message for i in issues if i.category == 'schema']
            self.assertTrue(any('missing creature_template entry 999999' in m for m in ref_msgs))
            self.assertTrue(any('missing quest_template ID 88888' in m for m in ref_msgs))
            self.assertTrue(any('missing item_template entry 777777' in m for m in ref_msgs))
            self.assertTrue(any('missing spell ID 555555' in m for m in ref_msgs))
            self.assertTrue(any('missing creature_template entry 666666' in m for m in ref_msgs))
            self.assertTrue(any('Unknown column `nonexistent_col` in INSERT INTO `creature`' in m for m in schema_msgs))

    def test_existing_agent_2026_10_31_corpus(self):
        agent_files = sorted((v.ROOT / 'sql/updates').rglob('2026_10_31_*.sql'))
        self.assertGreaterEqual(len(agent_files), 16)
        issues = v.validate_files(agent_files)
        self.assertEqual(issues, [])


if __name__ == '__main__':
    unittest.main()
