import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('no_binding_generator', Path(__file__).with_name('generate.py'))
generator = importlib.util.module_from_spec(spec)
spec.loader.exec_module(generator)


class GenerationTests(unittest.TestCase):
    def row(self, entry=1, **changes):
        row = dict(entry=entry, **{'class': 2}, bonding=1, quality=4, startquest=0, flags=0)
        row.update(changes)
        return row

    def test_preserves_every_protection(self):
        for changes in [{'class': 12}, {'bonding': 4}, {'bonding': 5}, {'quality': 6}, {'startquest': 1}]:
            self.assertFalse(generator.eligible(self.row(**changes), set()))
        self.assertFalse(generator.eligible(self.row(), {1}))
        for bonding in range(4):
            self.assertTrue(generator.eligible(self.row(bonding=bonding), set()))

    def test_update_is_scoped_and_preserves_other_flags(self):
        catalog = {1: self.row(1), 2: self.row(2, **{'class': 12}), 3: self.row(3)}
        sql = generator.character_sql(catalog, {3}, {70, 71})
        self.assertIn('WHERE (`flags` & 1) <> 0', sql)
        self.assertIn('    1)', sql)
        self.assertNotIn('    2', sql)
        self.assertNotIn('    3', sql)
        self.assertIn('`guid` NOT IN (70,71)', sql)
        for flags in [0, 1, 0x12345679, generator.UINT32_MAX]:
            changed = flags & 4294967294
            self.assertEqual(changed & ~1, flags & ~1)
            self.assertEqual(changed & 1, 0)
            self.assertEqual(changed & 4294967294, changed)

    def test_empty_eligibility_cannot_update_all_items(self):
        sql = generator.character_sql({1: self.row(**{'class': 12})}, set(), set())
        self.assertNotIn('UPDATE ', sql)

    def test_large_catalog_is_bounded_and_deterministic(self):
        catalog = {entry: self.row(entry) for entry in range(1, 2002)}
        sql = generator.character_sql(catalog, set(), set())
        self.assertEqual(sql.count('UPDATE '), 3)
        self.assertEqual(sql, generator.character_sql(dict(reversed(list(catalog.items()))), set(), set()))

    def test_catalog_validation_and_id_validation(self):
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'catalog.tsv'
            path.write_text('entry\tclass\tbonding\tquality\tstartquest\tflags\n1\t2\t1\t4\t0\t0\n')
            self.assertEqual(generator.read_catalog(path)[1], self.row())
            with path.open('a') as stream:
                stream.write('1\t2\t1\t4\t0\t0\n')
            with self.assertRaises(ValueError):
                generator.read_catalog(path)
        self.assertEqual(generator.ids('2, 1 2'), {1, 2})
        for bad in ['0', '-1', '4294967296', '1;2']:
            with self.assertRaises(ValueError):
                generator.ids(bad)

    def test_hotfix_update_retains_other_updates(self):
        sql = generator.world_sql('2026-11-01 00:00:00')
        self.assertIn('h.`entry` IS NULL', sql)
        self.assertIn('2442913102', sql)
        self.assertNotIn('DELETE', sql)
        self.assertNotIn('UPDATE `item_template`', sql)
        with self.assertRaises(ValueError):
            generator.world_sql("2026-11-01'; DROP TABLE item_template")


if __name__ == '__main__':
    unittest.main()
