# Shared DBC & DB2 Reader Library (`tools/dbc/`)

Dependency-free Python 3.9+ reader library for MoP 5.4.8 (build 18414) `WDBC` (`.dbc`) and `WDB2`
(`.db2`) client data stores (`T-030`).

## Purpose

- Parses binary `WDBC` (20-byte header) and `WDB2` (32/48-byte header + extended index/string
  lookup table when `max_index > 0`) files directly from `I:\UltimateMoP\gamedata\dbc` (and
  locale subdirectories like `dbc/enUS/`).
- Supports named schemas (`TableSchema`), TrinityCore format strings (`DBCfmt.h` / `DB2fmt.h`
  syntax: `n`, `d`, `i`, `f`, `s`, `b`, `l`, `x`, `X`), or automatic raw column inference.
- Provides O(1) primary-key lookups (`table[id]`, `table.get(id)`), lazy `DbcRecord` decoding,
  cached string-table lookups, and C-speed `struct.Struct.iter_unpack` tuple iteration.

## Interface

```python
from tools.dbc import DbcDirectory

dbc = DbcDirectory()  # reads MOP_DBC_DIR or defaults to I:/UltimateMoP/gamedata/dbc (locale='enUS')

spells = dbc.load("Spell.dbc")
fireball = spells[133]
print(fireball.id, fireball.name, fireball.spell_misc_id)

items = dbc.load("Item-sparse.db2")
ashbringer = items[13262]
print(ashbringer.name, ashbringer.item_level, ashbringer.quality)
```

### Custom TrinityCore Format Strings

```python
from tools.dbc import DbcTable

table = DbcTable.from_file("I:/UltimateMoP/gamedata/dbc/SpellIcon.dbc", fmt="ns")
print(table[1].col_1)
```

## Registered Schemas (`tools/dbc/schemas.py`)

| Schema Name | File | Fields | Primary Purpose |
|---|---|---|---|
| `Spell` | `Spell.dbc` | 25 | Spell names, ranks, descriptions, tooltips, component IDs |
| `SpellMisc` | `SpellMisc.dbc` | 26 | Spell icons, `SpellVisual[2]`, school mask, attributes, indices |
| `SpellEffect` | `SpellEffect.dbc` | 30 | Effect types, aura types, base points, trigger spells, misc values |
| `SpellIcon` | `SpellIcon.dbc` | 2 | Spell icon file paths (`Interface\Icons\...`) |
| `SpellCooldowns` | `SpellCooldowns.dbc` | 6 | Recovery times and category cooldowns |
| `SpellLevels` | `SpellLevels.dbc` | 6 | Base, max, and spell levels |
| `SpellDuration` | `SpellDuration.dbc` | 4 | Aura/effect durations in ms |
| `SpellRange` | `SpellRange.dbc` | 8 | Hostile/friendly min and max ranges |
| `SpellCastTimes` | `SpellCastTimes.dbc` | 4 | Cast times in ms |
| `SpellCategories` | `SpellCategories.dbc` | 10 | Category, dispel type, mechanic, damage class |
| `SpellItemEnchantment` | `SpellItemEnchantment.dbc` | 26 | Weapon enchants and `aura_id` -> `ItemVisuals.dbc` |
| `ItemVisuals` | `ItemVisuals.dbc` | 6 | 5 weapon attachment slots -> `ItemVisualEffects.dbc` |
| `ItemVisualEffects` | `ItemVisualEffects.dbc` | 2 | Weapon glow `.mdx` paths (`SPELLS\ENCHANTMENTS\...`) |
| `SpellVisual` | `SpellVisual.db2` | 30 | Precast, cast, impact, state, channel, missile visual kits |
| `SpellVisualKit` | `SpellVisualKit.db2` | 39 | Head, chest, base, hand, weapon, breath, world visual effects |
| `SpellVisualEffectName` | `SpellVisualEffectName.db2` | 9 | Visual effect `.mdx`/`.mdl` file paths and scales |
| `SpellVisualKitModelAttach` | `SpellVisualKitModelAttach.db2` | 15 | Attached visual effect models per `SpellVisualKit` |
| `SpellVisualMissile` | `SpellVisualMissile.db2` | 17 | Missile visual models and impact kits |
| `Item` | `Item.db2` | 8 | Item class, subclass, material, `display_id`, inventory type, sheath |
| `ItemSparse` | `Item-sparse.db2` | 134 | Item names, descriptions, quality, ilvl, stats, spells, sockets |
| `ItemDisplayInfo` | `ItemDisplayInfo.dbc` | 26 | Item icons, models, textures, `item_visual_id`, `spell_visual_id` |
| `Creature` | `Creature.db2` | 18 | Client creature entries, names, display IDs, scales |
| `CreatureDisplayInfo` | `CreatureDisplayInfo.dbc` | 20 | Creature display IDs, model ID, scale, skin textures, portrait |
| `CreatureModelData` | `CreatureModelData.dbc` | 34 | Creature `.m2` model paths, scale, collision/mount heights |
| `CreatureFamily` | `CreatureFamily.dbc` | 12 | Creature family names, scales, icons |
| `CreatureType` | `CreatureType.dbc` | 3 | Creature type names (Beast, Dragonkin, etc.) |
| `GameObjectDisplayInfo` | `GameObjectDisplayInfo.dbc` | 21 | GameObject `.mdx`/`.wmo` paths and bounding boxes |
| `GameObjects` | `GameObjects.db2` | 17 | Client GameObject entries, display IDs, names |

## Configuration

- `MOP_DBC_DIR`: Path to the directory containing `.dbc` and `enUS/*.db2` files (defaults to
  `I:/UltimateMoP/gamedata/dbc`).

## Unit Tests

```powershell
python -m unittest discover -s tools/dbc -v
```
