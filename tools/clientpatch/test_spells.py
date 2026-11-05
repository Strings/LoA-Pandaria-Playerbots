import struct
import sys
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
sys.path.insert(0, str(HERE))

import spells  # noqa: E402
from dbcfile import Dbc  # noqa: E402


def table(fields, rows, strings=b'\0'):
    raw = struct.pack('<4sIIII', b'WDBC', len(rows), fields, fields * 4, len(strings))
    for row in rows:
        raw += struct.pack('<%dI' % fields, *row)
    return raw + strings


def source_tables():
    spell = [0] * 25
    spell[0], spell[1], spell[13], spell[24] = 500, 1, 77, 900  # name "Old", categories 77, misc row 900
    misc = [0] * 26
    misc[0], misc[1], misc[5], misc[8], misc[23] = 900, 500, 0x20, 0x200, 42  # attributes_ex2, ex5, icon
    effects = []
    for index in range(2):
        effect = [0] * 30
        effect[0], effect[2], effect[6], effect[27], effect[28] = 7000 + index, 6, (-5 + index) & 0xFFFFFFFF, 500, index
        effects.append(effect)
    other = [0] * 30
    other[0], other[1], other[27] = 7100, 1, 500  # a heroic-difficulty row, not cloned
    return {'Spell': Dbc(table(25, [spell], b'\0Old\0'), 'Spell.dbc'),
            'SpellMisc': Dbc(table(26, [misc]), 'SpellMisc.dbc'),
            'SpellEffect': Dbc(table(30, effects + [other]), 'SpellEffect.dbc')}


EDIT = {'type': 'spell', 'id': 310100, 'from': 500, 'name': "Crane's Plume", 'rank': 'Celestial Gift',
        'description': 'Heals.', 'icon': 6359, 'attributes': {'attributes_ex2': {'set': 0x4}, 'attributes_ex5': {'clear': 0x200}}}


class CloneTests(unittest.TestCase):
    def test_clone_rows(self):
        tables = source_tables()
        rows = spells.clone(EDIT, tables)
        spell, misc = rows['Spell'][0], rows['SpellMisc'][0]
        self.assertEqual((spell[0], spell[13], spell[24]), (310100, 77, 310100))
        self.assertEqual(tables['Spell'].string(spell[1]), "Crane's Plume")
        self.assertEqual(tables['Spell'].string(spell[3]), 'Heals.')
        self.assertEqual(spell[4], 0)
        self.assertEqual((misc[0], misc[1], misc[23], misc[5], misc[8]), (310100, 310100, 6359, 0x24, 0))
        self.assertEqual([(e[0], e[27], e[28]) for e in rows['SpellEffect']], [(3101000, 310100, 0), (3101001, 310100, 1)])
        # The source stays as it was.
        self.assertEqual(tables['Spell'].string(tables['Spell'].find(0, 500)[0][1]), 'Old')

    def test_interrupts_copy_clears_bits(self):
        tables = source_tables()
        tables['Spell'].rows[0][17] = 27126
        tables['SpellInterrupts'] = Dbc(table(8, [[27126, 500, 0, 0x18, 0, 0x18, 0, 0x28]]), 'SpellInterrupts.dbc')
        rows = spells.clone(dict(EDIT, interrupts={'clear_aura': 0x18, 'clear_channel': 0x18}), tables)
        self.assertEqual(rows['SpellInterrupts'], [[27127, 310100, 0, 0, 0, 0, 0, 0x28]])
        self.assertEqual(rows['Spell'][0][17], 27127)
        self.assertEqual(rows['ServerSpell'][0][17], 27126)   # the server's data files only have the source row

    def test_cast_time_index(self):
        tables = source_tables()
        self.assertEqual(spells.clone(EDIT, tables)['SpellMisc'][0][17], 0)   # the source's own
        tables = source_tables()
        self.assertEqual(spells.clone(dict(EDIT, cast_time_index=1), tables)['SpellMisc'][0][17], 1)

    def test_duration_and_effect_overrides(self):
        tables = source_tables()
        rows = spells.clone(dict(EDIT, duration_index=31, effects={'1': {'base_points': 45, 'misc_value': -3}}), tables)
        self.assertEqual(rows['SpellMisc'][0][18], 31)
        first, second = rows['SpellEffect']
        self.assertEqual(first[6], (-5) & 0xFFFFFFFF)            # index 0 keeps the source's base points
        self.assertEqual((second[6], second[13]), (45, (-3) & 0xFFFFFFFF))
        # The source effect row is untouched.
        self.assertEqual(tables['SpellEffect'].find(0, 7001)[0][6], (-4) & 0xFFFFFFFF)

    def test_skill_line_puts_clone_in_spellbook(self):
        tables = source_tables()
        abilities = [[27083, 183, 125439, 0, 0, 1] + [0] * 7, [27090, 183, 125440] + [0] * 10,
                     [30000, 200, 999999] + [0] * 10]
        tables['SkillLineAbility'] = Dbc(table(13, abilities), 'SkillLineAbility.dbc')
        tables['SkillLineAbilitySortedSpell'] = Dbc(
            table(2, [[27083, 125439], [27090, 125440], [30000, 999999]]), 'SkillLineAbilitySortedSpell.dbc')
        rows = spells.clone(dict(EDIT, skill_line=183), tables)
        # Copies the line's last row, takes the next free id and stays inside the line's block.
        self.assertEqual(rows['SkillLineAbility'], [[30001, 183, 310100] + [0] * 10])
        self.assertEqual([r[0] for r in tables['SkillLineAbility'].rows], [27083, 27090, 30001, 30000])
        # The sorted table grows with it, still sorted by spell.
        self.assertEqual(tables['SkillLineAbilitySortedSpell'].rows,
                         [[27083, 125439], [27090, 125440], [30001, 310100], [30000, 999999]])
        with self.assertRaises(KeyError):
            spells.clone(dict(EDIT, id=310101, skill_line=999), tables)

    def test_cooldown_and_visual(self):
        tables = source_tables()
        tables['Spell'].rows[0][spells.S_COOLDOWNS] = 14174
        tables['SpellCooldowns'] = Dbc(table(6, [[14174, 500, 0, 5000, 2000, 1500], [14200, 1, 0, 0, 0, 0]]),
                                       'SpellCooldowns.dbc')
        rows = spells.clone(dict(EDIT, cooldown=30000, visual=310103), tables)
        # Own row with the next id: the given cooldown, no category cooldown, the source's global cooldown.
        self.assertEqual(rows['SpellCooldowns'], [[14201, 310100, 0, 0, 30000, 1500]])
        self.assertEqual(rows['Spell'][0][spells.S_COOLDOWNS], 14201)
        self.assertEqual(rows['SpellMisc'][0][spells.M_VISUAL], 310103)
        self.assertEqual(len(tables['SpellCooldowns'].rows), 3)

    def test_round_trip(self):
        tables = source_tables()
        spells.clone(EDIT, tables)
        again = Dbc(tables['Spell'].to_bytes(), 'Spell.dbc')
        self.assertEqual(again.string(again.find(0, 310100)[0][2]), 'Celestial Gift')
        self.assertEqual(len(Dbc(tables['SpellEffect'].to_bytes()).rows), 5)

    def test_refusals(self):
        tables = source_tables()
        with self.assertRaises(KeyError):
            spells.clone(dict(EDIT, **{'from': 501}), tables)
        spells.clone(EDIT, tables)
        with self.assertRaises(ValueError):
            spells.clone(EDIT, tables)

    def test_server_sql(self):
        tables = source_tables()
        rows = [spells.clone(EDIT, tables)]
        sql = spells.server_sql([EDIT], rows)
        self.assertIn("(310100, 0, 0, 77, 0, 0, 0, 0, 0, 310100, 'Crane''s Plume (clone of 500)')", sql)
        self.assertIn('DELETE FROM `spelleffect_dbc` WHERE `EffectSpellId` IN (310100);', sql)
        # Signed base points survive as signed values.
        self.assertIn('(3101000, 6, 0.0, 0, 0, -5,', sql)


if __name__ == '__main__':
    unittest.main()
