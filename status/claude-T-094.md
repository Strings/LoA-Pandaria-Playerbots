# T-094: Multi-classing framework and the first two second classes

**Agent:** claude
**Branch:** claude/T-094-multiclass
**Task status:** review

## What changed
- `src/server/scripts/Custom/Multiclass/`: a base class takes one active second class that levels on its own
  (experience mirrored from the character's kills and quests, quests at the cap included; each second-class level
  costs `Multiclass.XpPct` of the character's next level). Free swapping out of combat; set-aside classes keep their level.
- The rules engine runs interaction rows (`multiclass_rule`): trigger (cast, ability hit, killing blow), condition
  (aura on target, aura on self, target attacking someone else), and effect (cast on target or self, energize),
  filtered by base spec and second-class level, each with its own cooldown.
- Ability damage is a share of the Gift power (`multiclass_ability.PowerPct`). Cost is a % of max power and the
  cooldown comes from the row.
- Barbarian (Warrior) and Necromancer (Warlock): three abilities each, with per-spec interactions (DESIGN.md Multi-classing).
- `addons/Multiclass/`: an Alt-N panel (also `/mc`) that lists the classes the base class can take, with level and XP,
  Make active / Set aside, and the active class's spellbook (drag to bars).
- Spell clones 320000–320005 in `tools/clientpatch/edits.json`. The tool gained `range_index` edits and clamps
  unsigned float columns.
- The headless client takes `--char NAME`.

## How it was tested
- Unit tests in `tests/multiclass/`: experience per level, level-ups, rule cooldowns.
- Regression scenarios:
  - `multiclass` (Seedtest, Warrior): class gate, switching, XP to 30, then Arms Hamstring followed by Hurl Axe casts
    Net (rule 1), and unlearning on set-aside.
  - `multiclass-necromancer` (Seedlock, Warlock): Demonology Bone Spear grants Demonic Fury (rule 8).
- Not yet verified live: the Fury, Protection, Affliction, Destruction and Frenzy rules.

## Decisions made
- Second classes are kits, not client classes (same reasoning as fusion-server F-021). Abilities are learned spells,
  unlearned when the class is set aside.
- Barbarian runs on rage and Necromancer on mana; no new resource yet.
- An unlock chat message at 30 ("Press Alt-N") stands in for the intro quest.

## For the other agent
- SQL:
  - world `2026_11_05_claude_02_multiclass.sql`, plus the regenerated `2026_11_03_claude_00_heartseed_gift_spells.sql`
    (24 spells).
  - characters `2026_11_05_claude_00_character_multiclass.sql`.
  - auth `2026_11_05_claude_00_multiclass_commands.sql` (RBAC 100000/100001).
- Config: `Multiclass.Enable`, `Multiclass.UnlockLevel`, `Multiclass.XpPct` in worldserver.conf.dist.
- Clones from creature spells can carry player-only attributes or knockbacks. Check `SPELL_ATTR3_ONLY_TARGET_PLAYERS`.
- The rare 5824 (with affixes) would not keep Hamstring; tests use Mottled Boar 3098.

## Next
- Intro quests per class order, playerbots strategies, custom icons (ART-NEEDED row), Loadouts saving the active second
  class, the remaining class table, and new resources where a class needs one.

## Map additions
- `Custom/Multiclass/`, `tests/multiclass/`, `addons/Multiclass/` (added).
