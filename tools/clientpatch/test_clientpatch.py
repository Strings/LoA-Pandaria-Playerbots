import struct
import zlib
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
        self.assertEqual((magic, size, total, version), (b'MPQ\x1a', 44, len(archive), 1))

    def test_attributes_cover_every_block(self):
        archive = mpq.build({'a': b'1', 'b': b'22'}, 'base')
        raw = mpq.read(archive, '(attributes)')
        version, flags = struct.unpack_from('<II', raw)
        # a, b, (patch_metadata), (listfile), (attributes): CRC32, file time and MD5 for each.
        self.assertEqual((version, flags, len(raw)), (100, 7, 8 + 5 * (4 + 8 + 16)))
        self.assertEqual(struct.unpack_from('<I', raw, 8)[0], zlib.crc32(b'1'))


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
                # Spell clones live in a reserved custom spell block (310000 and up, docs/ID_RANGES.md).
                self.assertTrue(310000 <= edit["id"] <= 399999 and edit["name"], edit["id"])
            elif edit['type'] == 'row':
                self.assertTrue(get_schema(edit['file']), edit['file'])
                self.assertTrue(310000 <= edit['id'] <= 310999, edit['id'])
            else:
                self.assertNotEqual(edit['from'], edit['to'])
            self.assertTrue(edit['why'])


def wdb2(rows, fields):
    body = b''.join(struct.pack('<%dI' % fields, *row) for row in rows)
    header = struct.pack('<4sIIII', b'WDB2', len(rows), fields, fields * 4, 1)
    return header + struct.pack('<IIIiiiI', 0xABCD, 18273, 0, 0, 0, -1, 0) + body + b'\0'


class RowEditTests(unittest.TestCase):
    schema = get_schema('SpellVisualKitModelAttach.db2')

    def test_copies_row_to_new_id_in_wdb2(self):
        source = [18040, 30429, 7, 17, 0, 0, 0, 0, 0, 0, 0, -1 & 0xFFFFFFFF, -1 & 0xFFFFFFFF, 0, 0]
        raw = wdb2([source], 15)
        out = build.add_rows(raw, self.schema, [{'id': 310103, 'from': 18040,
                                                 'set': {'parent_spell_visual_kit_id': 310103, 'scale': 1.5}}])
        # Header extras (hash, build, locale) survive; the copy changes only what was set.
        self.assertEqual(out[20:48], raw[20:48])
        copy = list(struct.unpack_from('<15I', out, 48 + 60))
        self.assertEqual(copy[:3], [310103, 310103, 7])
        self.assertEqual(struct.unpack('<f', struct.pack('<I', copy[10]))[0], 1.5)
        self.assertEqual(out[48:108], raw[48:108])

    def test_refuses_missing_source_and_unknown_column(self):
        raw = wdb2([[1] + [0] * 14], 15)
        with self.assertRaises(KeyError):
            build.add_rows(raw, self.schema, [{'id': 310103, 'from': 2}])
        with self.assertRaises(KeyError):
            build.add_rows(raw, self.schema, [{'id': 310103, 'from': 1, 'set': {'nope': 1}}])


if __name__ == '__main__':
    unittest.main()
