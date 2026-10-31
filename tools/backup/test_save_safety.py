from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch
import save_safety as s


class Tests(unittest.TestCase):
    def test_custom_table_boundary(self):
        good = "-- comment\nCREATE TABLE IF NOT EXISTS `heartseed_state` (`guid` INT, note TEXT DEFAULT 'USE world;');"
        self.assertEqual(s.validate_migration(good, ['heartseed_state']), good)
        for sql in ['USE characters;', 'DROP TABLE heartseed_state;',
                    'UPDATE characters SET level=90;', 'UPDATE heartseed_state, characters SET level=90;',
                    'INSERT INTO heartseed_state SELECT * FROM characters;',
                    'CREATE TABLE IF NOT EXISTS world.heartseed_state (id INT);',
                    '/*!50000 DROP DATABASE world */;', '\\! echo hello',
                    'DELETE FROM heartseed_state USING characters;']:
            with self.subTest(sql=sql), self.assertRaises(ValueError):
                s.validate_migration(sql, ['heartseed_state'])

    def test_paths_and_restore_integrity(self):
        temp_root = s.REPO / 'build/test-temporary'
        temp_root.mkdir(parents=True, exist_ok=True)
        with tempfile.TemporaryDirectory(dir=temp_root) as name:
            root = Path(name).resolve()
            with patch.object(s, 'REPO', root):
                snap = root / 'snapshot'
                (snap / 'data').mkdir(parents=True)
                (snap / 'data/test.ibd').write_bytes(b'save data')
                import json
                (snap / 'manifest.json').write_text(json.dumps({'format': s.MARKER, 'files': s.files_manifest(snap / 'data')}))
                restored = s.restore(snap, root / 'restored')
                self.assertEqual((restored / 'test.ibd').read_bytes(), b'save data')
                with self.assertRaises(ValueError):
                    s.restore(snap, restored)
                (snap / 'data/test.ibd').write_bytes(b'corrupt')
                with self.assertRaises(ValueError):
                    s.restore(snap, root / 'another')
                with self.assertRaises(ValueError):
                    s.owned_path(root.parent / 'outside')


if __name__ == '__main__':
    unittest.main()
