import struct
import sys
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import build  # noqa: E402
import mpq  # noqa: E402
from tools.dbc.schemas import get_schema  # noqa: E402


def spell_levels(rows):
    body = b''.join(struct.pack('<6I', *row) for row in rows)
    return struct.pack('<4sIIII', b'WDBC', len(rows), 6, 24, 1) + body + b'\0'


class MpqTests(unittest.TestCase):
    def test_round_trip(self):
        files = {'DBFilesClient\\SpellLevels.dbc': b'abc' * 1000, 'Interface\\Icons\\X.blp': b'icon'}
        archive = mpq.build(files)
        for name, data in files.items():
            self.assertEqual(mpq.read(archive, name), data)
        # Lookups ignore case and slash direction, like the client.
        self.assertEqual(mpq.read(archive, 'dbfilesclient/spelllevels.dbc'), files['DBFilesClient\\SpellLevels.dbc'])
        self.assertEqual(sorted(mpq.listfile(archive)), sorted(files))
        with self.assertRaises(KeyError):
            mpq.read(archive, 'missing.dbc')

    def test_known_hash(self):
        # Reference value from the MPQ format documentation for the hash table key.
        self.assertEqual(mpq.hash_string('(hash table)', mpq.HASH_KEY), 0xC3AF3770)

    def test_header(self):
        archive = mpq.build({'a': b'1'})
        magic, size, total, version = struct.unpack_from('<4sIIH', archive)
        self.assertEqual((magic, size, total, version), (b'MPQ\x1a', 32, len(archive), 0))


class DbcPatchTests(unittest.TestCase):
    schema = get_schema('SpellLevels.dbc')


    def test_update_archive_names_its_chain(self):
        self.assertEqual(mpq.read(mpq.build({'a': b'1'}, 'enUS'), '(patch_metadata)'), b'enUS')
        with self.assertRaises(KeyError):
            mpq.read(mpq.build({'a': b'1'}), '(patch_metadata)')
    def test_edits_one_cell(self):
        raw = spell_levels([(6757, 1, 0, 0, 0, 5), (6758, 33388, 0, 0, 0, 20)])
        out = build.patch_dbc(raw, self.schema, [{'id': 6758, 'field': 'spell_level', 'from': 20, 'to': 1}])
        self.assertEqual(out, spell_levels([(6757, 1, 0, 0, 0, 5), (6758, 33388, 0, 0, 0, 1)]))

    def test_refuses_unexpected_source_value(self):
        raw = spell_levels([(6758, 33388, 0, 0, 0, 30)])
        with self.assertRaises(ValueError):
            build.patch_dbc(raw, self.schema, [{'id': 6758, 'field': 'spell_level', 'from': 20, 'to': 1}])

    def test_refuses_missing_row(self):
        with self.assertRaises(KeyError):
            build.patch_dbc(spell_levels([(1, 1, 0, 0, 0, 1)]), self.schema,
                            [{'id': 6758, 'field': 'spell_level', 'from': 20, 'to': 1}])

    def test_manifest_edits_have_reasons(self):
        manifest = build.load_manifest()
        self.assertEqual(sorted(build.archive_names(manifest).values()),
                         ['enUS\\wow-update-enUS-18414.MPQ', 'wow-update-base-18414.MPQ'])
        for edit in manifest['edits']:
            if edit.get('type', 'dbc') == 'dbc':
                self.assertTrue(get_schema(edit['file']), edit['file'])
            elif edit['type'] == 'spell':
                # Spell clones live in the reserved custom block (docs/ID_RANGES.md).
                self.assertTrue(310000 <= edit['id'] <= 310999 and edit['name'], edit['id'])
            else:
                self.assertNotEqual(edit['from'], edit['to'])
            self.assertTrue(edit['why'])


if __name__ == '__main__':
    unittest.main()
