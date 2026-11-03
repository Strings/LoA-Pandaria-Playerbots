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

- `edits.json` names the archive and lists every edit with what it replaces (`from`), the new value (`to`) and why.
  A `from` that no longer matches the source fails the build.
  - `dbc` edits change one cell (`id`, `field`) of a table from the extracted server game data (`--dbc`, else
    `MOP_DBC_DIR`, else `<repo>/gamedata/dbc`); field names come from the shared DBC schemas (`tools/dbc/`).
  - `text` edits replace one exact, unique string in a client file such as interface Lua. The current file is read
    from the client's own archives (`--client`, else `MOP_CLIENT_DIR`, else the `mop-clean` folder next to the
    repo) by `client.py`, which decompresses Blizzard's archives and applies their `PTCH` delta patches (COPY and
    BSD0, MD5-checked) in load order. It never writes into the client.
- `mpq.py` writes format version 1 archives with each file stored uncompressed in plain sectors, the layout Blizzard's update archives use (the client fails on uncompressed single-unit files). No third-party
  dependency.
- The archive is `Data/wow-update-base-18414.MPQ`: the client loads `wow-update-base-<build>` archives in build
  order, so the one numbered after the newest Blizzard update (18273) loads last and wins.

Only WDBC tables with 4-byte fields and text files are edited so far; DB2 tables and art come with their first use.

## Edits

| File | Change | Why |
|---|---|---|
| `Interface\AddOns\Blizzard_PetJournal\Blizzard_PetJournal.lua` | `PLAYER_MOUNT_LEVEL` 20 to 1 | The mount journal greys out every mount and refuses drags below this hard-coded level, whatever the character can ride; the server lets everyone ride from 1 (T-011) |

## Tests

`python -m unittest discover -s tools/clientpatch -p test_*.py`: archive round trip, the format's reference hash,
single-cell DBC edits, text edits, Blizzard delta patches (COPY, BSD0 with and without RLE, sign-magnitude seeks, MD5 checks)
and the refusals.
