# T-086: Gift identity

**Agent:** claude
**Branch:** claude/T-086-gift-identity
**Task status:** review (waiting on a real-client look at the names, tooltips and icons)

## What changed
- The four Celestial Gifts and the five rider auras are their own spells: clones of the client spells they borrowed,
  with the same effects and visuals but their own names, tooltips and placeholder icons. Call of Xuen 310100,
  Crane's Plume 310101, Niuzao's Retort 310102, Serpent's Breath 310103; Ferocity of Xuen, Crane's Swiftness,
  Niuzao's Shield, Humbled by Niuzao and Jade Fire 310110 to 310114. Berserking, Sprint, Ice Barrier, Invoke Xuen
  and the other sources are untouched for the classes and races that own them.
- `tools/clientpatch`: new `spell` edit (clone a client spell with a name, rank, description, aura description,
  icon and attribute changes) and `build.py sql`, which writes the same spells as server SQL
  (`2026_11_03_claude_00_heartseed_gift_spells.sql`, generated). `dbcfile.py` edits whole tables with strings.
- Heartseed: `heartseed_gift` and `heartseed_gift_rider` point at the clones; scripts are bound to them (the monk and
  cloak class scripts too). The login sync takes back the old borrowed spells (`RETIRED_GIFT_SPELLS`), except a
  talent a monk took. The "Gift wakes" message uses the trait's name.
- Characters update: anyone holding an old Gift spell gets its clone, and their action bar buttons move to it.
- Core fix (`DBCfmt.h`): `spelleffect_dbc` now fills MiscValueB from its `EffectMiscValueB` column; before, it
  filled PointsPerComboPoint, so every SQL-defined summon lost its summon properties (47 upstream rows affected).
- `docs/KNOWLEDGE.md`: the custom spell tables are read by position; clone instead of renaming.

## How it was tested
- Unit tests 20/20 in `tools/clientpatch` (clone rows, strings, attributes, effect ids, round trip, server SQL).
- Scenarios heartseed-gifts, heartseed-riders and heartseed-xuen pass against the clones (the tiger summons from the
  clone); full suite run before the PR.

## Decisions made
- Clones, not renamed borrowed spells: a rename in the client would rename the real ability for everyone.
- Clones cost nothing and have no server-side cooldown row; the Heartseed sets Gift cooldowns and amounts as before.
- Placeholder icons from the client's own set until custom art exists (`icon` in `edits.json`).

## For the other agent
- A new player-visible custom spell: add a `spell` edit to `tools/clientpatch/edits.json`, run `build.py sql` and
  commit the regenerated SQL. Use ids in your system's custom spell block.
- Upstream `spelleffect_dbc` summons now get their MiscValueB. If a scripted summon changes behaviour, that's why.

## Next
- Custom icon art (BLP writer, SpellIcon rows) once the panel art lands.
- The triggered helper spells (Xuen's lightning 123996, the Breath tick 124947) still show their own names in the
  combat log.
