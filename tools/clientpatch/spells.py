"""Spell clones: a client spell copied to a new id with its own name, tooltip, icon and attributes.

A clone keeps the source's behaviour (effects, visuals, cast time, range, targeting) but shows as its own spell,
so a borrowed spell can be renamed without renaming the real ability it came from. The client gets the rows in
its Spell, SpellMisc and SpellEffect tables; the server gets the same spell as `spell_dbc`, `spellmisc_dbc` and
`spelleffect_dbc` rows (`server_sql`), which the core loads over its own tables.

Not copied: SpellPower (clones cost nothing), SpellEffectScaling (amounts come from base points; the server
script sets real amounts) and server-side cooldowns (the core's `spell_dbc` has no cooldown column).
"""
import bisect
import struct
from typing import Dict, List

from dbcfile import Dbc, as_float, as_int

# Column indexes, from tools/dbc/schemas.py.
S_ID, S_NAME, S_RANK, S_DESC, S_AURA_DESC, S_MISC = 0, 1, 2, 3, 4, 24
M_ID, M_SPELL, M_DIFFICULTY, M_ICON, M_ACTIVE_ICON = 0, 1, 2, 23, 24
S_COOLDOWNS = 15
E_ID, E_DIFFICULTY, E_SPELL = 0, 1, 27
M_VISUAL = 21
M_CAST_TIME = 17
M_RANGE = 19
M_DURATION = 18
# SpellEffect cells an edit may override, by name.
EFFECT_FIELDS = {'base_points': 6, 'aura': 4, 'misc_value': 13, 'trigger_spell': 23, 'radius_index': 16}
# Float cells: points_per_level adds that much per caster level on top of the base points.
FLOAT_EFFECT_FIELDS = {'points_per_level': 18}
CD_ID, CD_SPELL, CD_RECOVERY = 0, 1, 4
S_INTERRUPTS = 17
I_ID, I_SPELL, I_AURA, I_CHANNEL = 0, 1, 3, 5

MISC_ATTRIBUTES = ['attributes'] + ['attributes_ex%s' % s for s in [''] + [str(n) for n in range(2, 14)]]

# The core's custom tables and the client columns they come from. The core reads these tables by column order into
# the positions its Custom*fmt strings (DBCfmt.h) mark, whatever the columns are called: in `spell_dbc` the columns
# named Levels, TargetRestrictions and Interrupts fill Interrupts (17), Levels (18) and TargetRestrictions (21); in
# `spelleffect_dbc` the column named EffectDieSides fills ItemType (11), which no clone needs.
SERVER_SPELL = [('Id', 0), ('SpellAuraOptionsId', 10), ('SpellCastingRequirementsId', 12),
                ('SpellCategoriesId', 13), ('SpellClassOptionsId', 14), ('SpellEquippedItemsId', 16),
                ('SpellLevelsId', 17), ('SpellTargetRestrictionsId', 18), ('SpellInterruptsId', 21),
                ('SpellMiscId', 24)]
SERVER_MISC = ([('Id', 0)] + [('Attributes' if i == 0 else 'AttributesEx' + ('' if i == 1 else str(i)), 3 + i)
                               for i in range(14)]
               + [('CastingTimeIndex', 17), ('DurationIndex', 18), ('RangeIndex', 19), ('SchoolMask', 25)])
FLOAT, INT = 'f', 'i'
# The server's EffectValueMultiplier column is FLOAT UNSIGNED; a client value below zero (summons use -10) is clamped.
UFLOAT = 'uf'
SERVER_EFFECT = [('Id', 0), ('Effect', 2), ('EffectValueMultiplier', 3, UFLOAT), ('EffectApplyAuraName', 4),
                 ('EffectAplitude', 5), ('EffectBasePoints', 6, INT), ('EffectBonusMultiplier', 7, FLOAT),
                 ('EffectDamageMultiplier', 8, FLOAT), ('EffectChainTarget', 9), ('EffectDieSides', 11),
                 ('EffectMechanic', 12), ('EffectMiscValue', 13, INT), ('EffectMiscValueB', 14, INT),
                 ('EffectRadiusIndex', 16), ('EffectRadiusIndexMax', 17), ('EffectRealPointsPerLevel', 18, FLOAT),
                 ('EffectSpellClassMaskA', 19), ('EffectSpellClassMaskB', 20), ('EffectSpellClassMaskC', 21),
                 ('EffectSpellClassMaskD', 22), ('EffectTriggerSpell', 23), ('EffectImplicitTargetA', 25),
                 ('EffectImplicitTargetB', 26), ('EffectSpellId', 27), ('EffectIndex', 28), ('EffectAttributes', 29)]


SLA_ID, SLA_LINE, SLA_SPELL = 0, 1, 2
SORTED_ID, SORTED_SPELL = 0, 1


def effect_id(spell_id: int, index: int) -> int:
    """Clone effect rows get ids far above the client's own (about 230000)."""
    return spell_id * 10 + index


def clone(edit: dict, tables: Dict[str, Dbc]) -> Dict[str, List[List[int]]]:
    """Appends the clone's rows to `tables` (Spell, SpellMisc, SpellEffect; SpellCooldowns for a `cooldown`;
    SkillLineAbility and SkillLineAbilitySortedSpell for a `skill_line`) and returns them by table."""
    spell_dbc, misc_dbc, effect_dbc = tables['Spell'], tables['SpellMisc'], tables['SpellEffect']
    new_id, source_id = edit['id'], edit['from']
    sources = spell_dbc.find(S_ID, source_id)
    if not sources:
        raise KeyError('spell %d: no client spell to clone' % source_id)
    spell = list(sources[0])
    miscs = misc_dbc.find(M_ID, spell[S_MISC])
    if not miscs:
        raise KeyError('spell %d: no SpellMisc row %d' % (source_id, spell[S_MISC]))
    misc = list(miscs[0])

    spell[S_ID] = new_id
    spell[S_NAME] = spell_dbc.add_string(edit['name'])
    spell[S_RANK] = spell_dbc.add_string(edit.get('rank', ''))
    spell[S_DESC] = spell_dbc.add_string(edit.get('description', ''))
    spell[S_AURA_DESC] = spell_dbc.add_string(edit.get('aura_description', ''))
    spell[S_MISC] = new_id

    misc[M_ID], misc[M_SPELL] = new_id, new_id
    if 'visual' in edit:
        misc[M_VISUAL] = edit['visual']
    if 'icon' in edit:
        misc[M_ICON] = edit['icon']
        misc[M_ACTIVE_ICON] = 0
    # SpellCastTimes row; 1 is instant (a channel then starts at once).
    if 'cast_time_index' in edit:
        misc[M_CAST_TIME] = edit['cast_time_index']
    # SpellRange row (4 is 0 to 30 yd); a thrown weapon's own range has an 8 yd minimum.
    if 'range_index' in edit:
        misc[M_RANGE] = edit['range_index']
    # SpellDuration row (31 is 8 s, 18 is 20 s, 9 is 30 s).
    if 'duration_index' in edit:
        misc[M_DURATION] = edit['duration_index']
    for name, change in edit.get('attributes', {}).items():
        column = 3 + MISC_ATTRIBUTES.index(name)
        misc[column] = (misc[column] | change.get('set', 0)) & ~change.get('clear', 0)

    effects = []
    for row in sorted(effect_dbc.find(E_SPELL, source_id), key=lambda r: r[28]):
        if row[E_DIFFICULTY]:
            continue
        row = list(row)
        row[E_ID] = effect_id(new_id, row[28])
        row[E_SPELL] = new_id
        # `effects` overrides cells of one effect by its index, e.g. {"0": {"base_points": 45}}: how a borrowed
        # spell is scaled down. Amounts are base points; clones carry no effect scaling.
        for name, value in edit.get('effects', {}).get(str(row[28]), {}).items():
            if name in FLOAT_EFFECT_FIELDS:
                row[FLOAT_EFFECT_FIELDS[name]] = struct.unpack('<I', struct.pack('<f', float(value)))[0]
            else:
                row[EFFECT_FIELDS[name]] = value & 0xFFFFFFFF
        effects.append(row)
    if not effects:
        raise KeyError('spell %d: no effects to clone' % source_id)

    # The spellbook lists only spells with a SkillLineAbility row; without one the spell is known but has no tab,
    # so it can't be found or dragged. The row copies one already on that line (183 is the General tab). The client
    # also keeps SkillLineAbilitySortedSpell, (ability id, spell) sorted by spell, which it walks by
    # SkillLineAbility's row count: both tables must grow together or the client reads past the end and crashes.
    # Rows on one skill line stay together, and the new row's id continues the table's own sequence.
    skill_rows, skill_at = [], 0
    if 'skill_line' in edit:
        ability_dbc = tables['SkillLineAbility']
        lines = [r[SLA_LINE] for r in ability_dbc.rows]
        if edit['skill_line'] not in lines:
            raise KeyError('spell %d: no SkillLineAbility row on line %d to copy' % (new_id, edit['skill_line']))
        skill_at = len(lines) - lines[::-1].index(edit['skill_line'])
        row = list(ability_dbc.rows[skill_at - 1])
        row[SLA_ID] = max(r[SLA_ID] for r in ability_dbc.rows) + 1
        row[SLA_SPELL] = new_id
        skill_rows.append(row)

    # The tooltip's cooldown comes from the SpellCooldowns row; the server sets the real one (heartseed_gift), so
    # `cooldown` (ms) should match it. The row copies the source's, keeping its global cooldown.
    cooldown_rows = []
    if 'cooldown' in edit:
        cooldown_dbc = tables['SpellCooldowns']
        source = cooldown_dbc.find(CD_ID, spell[S_COOLDOWNS]) if spell[S_COOLDOWNS] else []
        row = list(source[0]) if source else [0] * cooldown_dbc.fields
        row[CD_ID] = max(r[CD_ID] for r in cooldown_dbc.rows) + 1
        row[CD_SPELL], row[CD_RECOVERY], row[CD_RECOVERY - 1] = new_id, edit['cooldown'], 0
        spell[S_COOLDOWNS] = row[CD_ID]
        cooldown_rows.append(row)

    # `interrupts` clears AuraInterruptFlags and ChannelInterruptFlags bits on the clone's own copy of the source's
    # SpellInterrupts row (8 = moving, 16 = turning). The server keeps the source row (its data files are stock), so
    # the core's spell corrections must clear the same bits; `server_spell` is the row the server SQL gets.
    interrupt_rows = []
    server_spell = list(spell)
    if 'interrupts' in edit:
        interrupt_dbc = tables['SpellInterrupts']
        source = interrupt_dbc.find(I_ID, spell[S_INTERRUPTS]) if spell[S_INTERRUPTS] else []
        if not source:
            raise KeyError('spell %d: no SpellInterrupts row to copy' % source_id)
        row = list(source[0])
        row[I_ID] = max(r[I_ID] for r in interrupt_dbc.rows) + 1
        row[I_SPELL] = new_id
        row[I_AURA] &= ~edit['interrupts'].get('clear_aura', 0)
        row[I_CHANNEL] &= ~edit['interrupts'].get('clear_channel', 0)
        spell[S_INTERRUPTS] = row[I_ID]
        interrupt_rows.append(row)

    spell_dbc.append(spell)
    misc_dbc.append(misc)
    for row in interrupt_rows:
        tables['SpellInterrupts'].append(row)
    for row in cooldown_rows:
        tables['SpellCooldowns'].append(row)
    for row in effects:
        effect_dbc.append(row)
    rows = {'Spell': [spell], 'SpellMisc': [misc], 'SpellEffect': effects, 'ServerSpell': [server_spell]}
    if interrupt_rows:
        rows['SpellInterrupts'] = interrupt_rows
    if cooldown_rows:
        rows['SpellCooldowns'] = cooldown_rows
    if skill_rows:
        tables['SkillLineAbility'].insert(skill_at, skill_rows[0])
        sorted_dbc = tables['SkillLineAbilitySortedSpell']
        pair = [skill_rows[0][SLA_ID], new_id]
        sorted_dbc.insert(bisect.bisect_right([r[SORTED_SPELL] for r in sorted_dbc.rows], new_id), pair)
        rows['SkillLineAbility'] = skill_rows
        rows['SkillLineAbilitySortedSpell'] = [pair]
    return rows


def _value(row, column, kind=None):
    if kind == FLOAT:
        return repr(round(as_float(row[column]), 6))
    if kind == UFLOAT:
        return repr(round(max(0.0, as_float(row[column])), 6))
    if kind == INT:
        return str(as_int(row[column]))
    return str(row[column])


def _insert(table, columns, rows, comments):
    names = ', '.join('`%s`' % c[0] for c in columns)
    comment = ', `Comment`' if comments else ''
    values = []
    for row, note in zip(rows, comments or [None] * len(rows)):
        cells = [_value(row, *c[1:]) for c in columns]
        if comments:
            cells.append("'%s'" % note.replace("'", "''"))
        values.append('(%s)' % ', '.join(cells))
    return 'INSERT INTO `%s` (%s%s) VALUES\n%s;\n' % (table, names, comment, ',\n'.join(values))


def server_sql(clones: List[dict], rows: List[Dict[str, List[List[int]]]]) -> str:
    """Idempotent SQL giving the server the same clones."""
    ids = ', '.join(str(c['id']) for c in clones)
    spells = [r.get('ServerSpell', r['Spell'])[0] for r in rows]
    miscs = [r['SpellMisc'][0] for r in rows]
    effects = [e for r in rows for e in r['SpellEffect']]
    names = ['%s (clone of %d)' % (c['name'], c['from']) for c in clones]
    return ''.join([
        '-- Generated by tools/clientpatch/build.py sql from edits.json; do not edit by hand.\n',
        '-- Spell clones: the same spells the client patch adds, so server and client agree.\n',
        'DELETE FROM `spell_dbc` WHERE `Id` IN (%s);\n' % ids,
        'DELETE FROM `spellmisc_dbc` WHERE `Id` IN (%s);\n' % ids,
        'DELETE FROM `spelleffect_dbc` WHERE `EffectSpellId` IN (%s);\n' % ids,
        _insert('spell_dbc', SERVER_SPELL, spells, names),
        _insert('spellmisc_dbc', SERVER_MISC, miscs, names),
        _insert('spelleffect_dbc', SERVER_EFFECT, effects, None),
        '-- The client knows these spells, so the core sends their auras (it treats spell_dbc ids as server-only).\n',
        'CREATE TABLE IF NOT EXISTS `spell_client_known` (\n'
        '  `spell_id` INT UNSIGNED NOT NULL,\n'
        '  PRIMARY KEY (`spell_id`)\n'
        ") ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COMMENT='Custom spells the client patch adds (tools/clientpatch)';\n",
        'DELETE FROM `spell_client_known` WHERE `spell_id` IN (%s);\n' % ids,
        'INSERT INTO `spell_client_known` (`spell_id`) VALUES %s;\n' % ', '.join('(%d)' % c['id'] for c in clones),
    ])
