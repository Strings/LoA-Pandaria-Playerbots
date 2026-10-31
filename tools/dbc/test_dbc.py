"""Unit and gamedata integration tests for tools/dbc/."""
from pathlib import Path
import struct
import tempfile
import unittest

import sys
ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from tools.dbc import (
    DbcDirectory,
    DbcTable,
    FieldSpec,
    SCHEMAS_BY_NAME,
    TableSchema,
    get_schema,
    parse_header,
    schema_from_trinity_fmt,
)


def build_synthetic_wdbc(rows: list, fmt: str = '<IifI', strings: tuple = ('Fireball', 'Frostbolt')) -> bytes:
    str_pool = bytearray(b'\x00')
    offsets = {}
    for s in strings:
        offsets[s] = len(str_pool)
        str_pool.extend(s.encode('utf-8') + b'\x00')
    rec_struct = struct.Struct(fmt)
    rec_bytes = bytearray()
    for r in rows:
        converted = [offsets[val] if isinstance(val, str) else val for val in r]
        rec_bytes.extend(rec_struct.pack(*converted))
    header = struct.pack('<4s4I', b'WDBC', len(rows), len(rows[0]), rec_struct.size, len(str_pool))
    return header + bytes(rec_bytes) + bytes(str_pool)


def build_synthetic_wdb2(rows: list, fmt: str = '<II', min_index: int = 10, max_index: int = 12) -> bytes:
    str_pool = bytearray(b'\x00TestItem\x00')
    rec_struct = struct.Struct(fmt)
    rec_bytes = bytearray()
    for r in rows:
        rec_bytes.extend(rec_struct.pack(*r))
    header = struct.pack(
        '<4s7I4i',
        b'WDB2',
        len(rows),
        len(rows[0]),
        rec_struct.size,
        len(str_pool),
        0x12345678,
        18414,
        1000,
        min_index,
        max_index,
        0,
        0,
    )
    index_block = b'\x00' * ((max_index - min_index + 1) * 6 if max_index != 0 else 0)
    return header + index_block + bytes(rec_bytes) + bytes(str_pool)


class DbcReaderTests(unittest.TestCase):
    def test_synthetic_wdbc_and_schema(self):
        schema = TableSchema(
            name='SyntheticSpell',
            filename='SyntheticSpell.dbc',
            fields=(
                FieldSpec('id', 'uint32'),
                FieldSpec('base_points', 'int32'),
                FieldSpec('speed', 'float'),
                FieldSpec('name', 'string'),
            ),
        )
        raw = build_synthetic_wdbc([
            (133, -25, 24.5, 'Fireball'),
            (116, 40, 28.0, 'Frostbolt'),
        ])
        table = DbcTable.from_bytes(raw, schema=schema)
        self.assertEqual(len(table), 2)
        self.assertIn(133, table)
        self.assertNotIn(999, table)

        rec = table[133]
        self.assertEqual(rec.id, 133)
        self.assertEqual(rec.base_points, -25)
        self.assertAlmostEqual(rec.speed, 24.5, places=4)
        self.assertEqual(rec.name, 'Fireball')
        self.assertEqual(rec['name'], 'Fireball')
        self.assertEqual(rec[3], 'Fireball')
        self.assertEqual(rec.get_int('base_points'), -25)
        self.assertEqual(rec.get_string('name'), 'Fireball')
        self.assertEqual(rec.to_dict()['name'], 'Fireball')

    def test_synthetic_wdb2_with_index_lookup_table_and_trinity_fmt(self):
        raw = build_synthetic_wdb2([
            (10, 1),
            (12, 1),
        ], fmt='<II', min_index=10, max_index=12)
        header = parse_header(raw)
        self.assertEqual(header.magic, 'WDB2')
        self.assertEqual(header.build, 18414)
        self.assertEqual(header.data_offset, 48 + 3 * 6)

        table = DbcTable.from_bytes(raw, fmt='ns', name='TestDb2')
        self.assertEqual(len(table), 2)
        self.assertEqual(table[10].col_1, 'TestItem')
        self.assertEqual(table[12]['col_1'], 'TestItem')

    def test_byte_fields_and_locale_fallback(self):
        with tempfile.TemporaryDirectory() as tmp:
            root = Path(tmp)
            (root / 'enUS').mkdir()
            # 0-byte placeholder in root (like SpellVisual.dbc in gamedata/dbc)
            (root / 'SpellIcon.dbc').write_bytes(b'')
            # Real file in enUS
            raw = build_synthetic_wdbc([(1, 'Fireball')], fmt='<II', strings=('Fireball',))
            (root / 'enUS' / 'SpellIcon.dbc').write_bytes(raw)

            dbc_dir = DbcDirectory(root, locale='enUS')
            table = dbc_dir.load('SpellIcon')
            self.assertEqual(len(table), 1)
            self.assertEqual(table[1].icon_path, 'Fireball')

    def test_all_registered_schemas_against_gamedata(self):
        gamedata_dbc = Path('I:/UltimateMoP/gamedata/dbc')
        if not gamedata_dbc.is_dir():
            self.skipTest('I:/UltimateMoP/gamedata/dbc not present')

        dbc_dir = DbcDirectory(gamedata_dbc, locale='enUS')
        seen = set()
        for schema in SCHEMAS_BY_NAME.values():
            if schema.name in seen:
                continue
            seen.add(schema.name)
            table = dbc_dir.load(schema.filename)
            self.assertGreater(len(table), 0, f'{schema.filename} should have records')
            first = table.record_at(0)
            as_dict = first.to_dict()
            self.assertEqual(tuple(as_dict.keys()), schema.field_names)

        # Spot-check Fireball (133) and Jade Spirit (4442)
        spells = dbc_dir.load('Spell.dbc')
        self.assertEqual(spells[133].name, 'Fireball')
        enchants = dbc_dir.load('SpellItemEnchantment.dbc')
        self.assertEqual(enchants[4442].description, 'Jade Spirit')
        self.assertEqual(enchants[4442].aura_id, 238)


if __name__ == '__main__':
    unittest.main()
