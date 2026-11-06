# Client patch (`tools/clientpatch/`)

Builds the update archives that carry every change we make to the 5.4.8 client's data, and installs them into a
client. DESIGN.md plans to ship all client changes as patch MPQs through a launcher; these are those archives, and
the launcher will install the same files.

## Interface

```
python tools/clientpatch/build.py build       # build/clientpatch/<archive> from edits.json
python tools/clientpatch/build.py check       # list the files inside the built archive
python tools/clientpatch/build.py sql         # server SQL for the spell clones
python tools/clientpatch/build.py install --client I:/UltimateMoP/mop-clean   # copies what build made; refuses if nothing is built
python tools/clientpatch/build.py uninstall --client I:/UltimateMoP/mop-clean
```

- `edits.json` names the client build and locale and lists every edit with what it replaces (`from`), the new value (`to`) and why.
  A `from` that no longer matches the source fails the build.
  - `dbc` edits change one cell (`id`, `field`) of a table from the extracted server game data (`--dbc`, else
    `MOP_DBC_DIR`, else `<repo>/gamedata/dbc`); field names come from the shared DBC schemas (`tools/dbc/`).
    WDB2 tables without an id index work too, and string columns take text for `from` and `to` (`Vignette.db2`).
  - `text` edits replace one exact, unique string in a client file such as interface Lua. The current file is read
    from the client's own archives (`--client`, else `MOP_CLIENT_DIR`, else the `mop-clean` folder next to the
    repo) by `client.py`, which decompresses Blizzard's archives and applies their `PTCH` delta patches (COPY and
    BSD0, MD5-checked) in load order. It never writes into the client.
  - `spell` edits clone a client spell to a new `id` (310000 block, `docs/ID_RANGES.md`) with its own `name`,
    `rank`, `description`, `aura_description`, `range_index` (a SpellRange row), `icon` (a SpellIcon id) and `attributes` changes (`set`/`clear`
    bits on SpellMisc), `cast_time_index` and `duration_index` (SpellCastTimes and SpellDuration rows) and per-effect `effects` overrides keyed by effect index (`base_points`, `points_per_level`, `aura`, `misc_value`, `trigger_spell`, `radius_index`). Clones carry no effect scaling, so `base_points` is the amount, plus `points_per_level` per caster level unless you zero it. The clone otherwise keeps the source's effects, visuals, cast time, range and targeting, so a
    borrowed spell can be renamed without renaming the ability it came from. `build.py sql` writes the same spells
    as `spell_dbc`, `spellmisc_dbc` and `spelleffect_dbc` rows into the file `server_sql` names; run it after any
    spell edit and commit the result. Not copied: power costs, effect scaling and server-side cooldowns (see
    `spells.py`). Port any `SpellMgr` corrections of the source into `attributes`.
- `dbcfile.py` holds a table as raw cells plus its string block, for cloning and appending rows.
- `mpq.py` writes format version 1 archives the way Blizzard stores files: 16 KB sectors behind a sector offset
  table, each zlib-compressed when that is smaller (flags `0x80000200`). The client fails on uncompressed
  single-unit files and on large uncompressed ones. No third-party dependency.
- The client patches each file only from update archives of the chain that holds it: `base` (`Data/`) or the
  locale (`Data/enUS/`). Each update archive names its chain in `(patch_metadata)`; without it the client
  attaches the archive to the wrong chain and its files silently never apply. So the build writes
  `Data/wow-update-base-18414.MPQ` and `Data/enUS/wow-update-enUS-18414.MPQ`, each tagged, and puts each file in
  the chain `client.py` finds it in. Numbered after the newest Blizzard update (18273), they load last and win.
- Editing a file of a `Blizzard_*` addon breaks its signed `.toc.sig` and the client refuses the addon as
  CORRUPT. The build ships a placeholder `.toc.sig` of the wrong size for each edited Blizzard addon, which the
  client treats as unsigned, so the addon loads (as tainted code, like any user addon).

Only tables with 4-byte fields (WDBC, and WDB2 without an id index) and text files are edited so far; art comes with
its first use.

`build.py sql` also lists every clone in `spell_client_known`. The core treats spell ids from 203751 up as
server-only and never sends their auras to the client; a listed spell is exempt, so a clone's buff shows.

## Edits

| File | Change | Why |
|---|---|---|
| `Interface\AddOns\Blizzard_PetJournal\Blizzard_PetJournal.lua` | `PLAYER_MOUNT_LEVEL` 20 to 1 | The mount journal greys out every mount and refuses drags below this hard-coded level, whatever the character can ride; the server lets everyone ride from 1 (T-011) |

## Tests

`python -m unittest discover -s tools/clientpatch -p test_*.py`: archive round trip, the format's reference hash,
single-cell DBC edits, text edits, Blizzard delta patches (COPY, BSD0 with and without RLE, sign-magnitude seeks, MD5 checks)
and the refusals.
