# Client patch (`tools/clientpatch/`)

Builds the one MPQ archive that carries every change we make to the 5.4.8 client's data, and installs it into a
client. DESIGN.md plans to ship all client changes as a patch MPQ through a launcher; this is that archive, and
the launcher will install the same file.

## Interface

```
python tools/clientpatch/build.py build       # build/clientpatch/<archive> from edits.json
python tools/clientpatch/build.py check       # list the files inside the built archive
python tools/clientpatch/build.py install --client I:/UltimateMoP/mop-clean
python tools/clientpatch/build.py uninstall --client I:/UltimateMoP/mop-clean
```

- `edits.json` names the archive and lists every edit: table, row ID, field, the value it replaces (`from`), the
  new value (`to`) and why. A `from` that no longer matches the source data fails the build.
- Source tables come from the extracted server game data (`--dbc`, else `MOP_DBC_DIR`, else `<repo>/gamedata/dbc`),
  which is the client's own latest data, and field names from the shared DBC schemas (`tools/dbc/`).
- `mpq.py` writes format version 1 archives with each file stored uncompressed in plain sectors, the layout Blizzard's update archives use (the client fails on uncompressed single-unit files). No third-party
  dependency.
- The archive is `Data/wow-update-base-18414.MPQ`: the client loads `wow-update-base-<build>` archives in build
  order, so the one numbered after the newest Blizzard update (18273) loads last and wins.

Only WDBC tables with 4-byte fields are edited so far; DB2 tables, icons and Lua frames come with their first use.

## Edits

| Table | Row | Change | Why |
|---|---|---|---|
| `SpellLevels.dbc` | 6758 | `spell_level` 20 to 1 | Apprentice Riding: the server teaches riding at level 1 (T-011); at 20 the client greys out every ground mount |

## Tests

`python -m unittest discover -s tools/clientpatch -p test_*.py`: archive round trip, the format's reference hash,
single-cell DBC edits and the refusals.
