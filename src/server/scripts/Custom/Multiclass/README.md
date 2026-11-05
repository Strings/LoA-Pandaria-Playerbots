# Multi-classing (`src/server/scripts/Custom/Multiclass/`)

T-094, docs/DESIGN.md "Multi-classing". A base class takes on a second class that levels on its own: Warrior /
Barbarian and Warlock / Necromancer so far. The active second class's abilities are learned into the spellbook up to
its level; switching unlearns them and learns the new class's. Interaction rules tie the second class to the base
class's specs.

## Interface

- `MulticlassRules.h`: pure rules (experience per level, leveling, rule cooldowns), unit-tested in `tests/multiclass`.
- `MulticlassMgr`: data (`LoadData`, also `.reload custom multiclass`), per-character state, experience, switching,
  the rules engine (`Fire`) and the addon sync.
- `multiclass_scripts.cpp`: hooks (login, experience, quests at the cap, level-ups, casts, killing blows), commands
  and the spell scripts: `spell_multiclass_ability` (cost, cooldown, damage from the Gift power, Hit rules) and
  `spell_multiclass_corpse_explosion`.
- Addon: `addons/Multiclass` (Alt-N).
- Spells: clones 320000+ in `tools/clientpatch/edits.json`, so the client has their names and tooltips.

## Data (world database)

- `multiclass_class`: id, name, `base_class_mask` (1 << (class id - 1)), `max_level`, description.
- `multiclass_ability`: `class_id`, `level` that teaches it, `spell_id`, `kind` (0 learned, 1 passive: rules only),
  `power_pct` (damage per hit, % of the Gift power; 0 keeps the spell's own), `cooldown` ms, `power_type` and
  `cost_pct` (% of the character's maximum of that power).
- `multiclass_rule`: `trigger` 1 cast of `trigger_spell` (not triggered), 2 hit of a second-class ability, 3 killing
  blow on a creature; `condition` 0 none, 1 target has `condition_spell` from you, 2 you have it, 3 target attacking
  someone else; `effect` 1 cast `effect_spell` on the target (triggered), 2 on yourself, 3 energize (`effect_spell`
  is the power type, `amount` the power); `base_spec` (0 any), `min_level` (second-class level), `cooldown` ms.
  Rules never chain: triggered casts start no rules.

Characters: `character_multiclass` (guid, class_id, level, xp, active).

## Experience

The active second class earns a copy of the experience the character earns (never a split), including quest
experience at the level cap. One second-class level costs `Multiclass.XpPct` % of the character's own next level.

## Config

`Multiclass.Enable` (1), `Multiclass.UnlockLevel` (30), `Multiclass.XpPct` (25) in `worldserver.conf.dist`.

## GM commands

| Command | |
|---|---|
| `.multiclass info` | the target's second classes, levels, learned abilities |
| `.multiclass set <class> <level> [xp]` | set a level (adds the class) |
| `.multiclass xp <amount>` | experience into the active second class |
| `.multiclass switch <class or none>` | switch, ignoring level and combat |
| `.multiclass clear` | remove every second class |
| `.multiclass rules [class id]` | list interaction rules |

Players: `.mc sync` and `.mc switch <class or none>` (the addon's).

## First two classes

| Class | Lvl | Ability | Interactions |
|---|---|---|---|
| Barbarian | 1 | Hurl Axe (320000) | Arms roots a Hamstrung target, Fury throws a second axe while Enraged, Protection taunts |
| Barbarian | 5 | War Cry (320001) | |
| Barbarian | 10 | Frenzy, passive (320002) | Mortal Strike, Bloodthirst or Shield Slam |
| Necromancer | 1 | Raise Skeleton (320003) | Affliction: a kill with your Corruption on the target raises skeletons |
| Necromancer | 5 | Bone Spear (320004) | Demonology: 60 Demonic Fury |
| Necromancer | 10 | Corpse Explosion (320005) | Destruction: enemies hit catch Immolate |

Not yet: the unlock quest from each class's order, bot strategies, new resources, custom icons (docs/ART-NEEDED.md).
