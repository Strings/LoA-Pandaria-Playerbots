# T-085: Client patch archive

**Agent:** claude
**Branch:** claude/T-085-client-patch
**Task status:** done

## What changed
- `tools/clientpatch/`: `edits.json` lists every client edit (what it replaces, the new value, why). `build.py` builds
  them into one update archive per chain that has edits (`Data/wow-update-base-18414.MPQ`,
  `Data/enUS/wow-update-enUS-18414.MPQ`) and installs or removes them; `mpq.py` writes the archives, and `client.py`
  reads the client's own archives, including Blizzard's PTCH/BSD0 delta patches, to get the current version of a
  file before editing it. No dependencies.
- First edit: the mount journal's `PLAYER_MOUNT_LEVEL` drops from 20 to 1. Below that hard-coded level the journal
  greyed out every mount and refused drags to the bar, whatever the character could ride; the server lets everyone
  ride from 1 (T-011). Mounts now drag to the bar (the human confirmed in game).
- Edited `Blizzard_*` addons get a placeholder `.toc.sig`, so the client loads them unsigned instead of CORRUPT.
- The MountBar addon (PR 122) is removed from the repo and the client; the patch replaces it.
- AGENTS.md: the client folder may now hold our patch archives in `Data/` (the human approved).

## How it was tested
- Unit tests 16/16: archive round trip, the format's reference hash, patch metadata, DBC and text edits, Blizzard
  delta patches, the unsigned signature and the refusals.
- Real client: mounts drag from the journal to the bar on Irkzarn (level 2).
- Dead ends, recorded in `docs/KNOWLEDGE.md`: uncompressed single-unit files crash the client; an archive without
  `(patch_metadata)` attaches to the wrong chain and its files never apply; an edited Blizzard addon with its
  original signature fails as CORRUPT.

## Decisions made
- Update archives per chain, named after the client build so they load after Blizzard's last update.
- Edits name what they replace, so a source change fails the build instead of patching blindly.
- Text edits start from the client's own current file, rebuilt through Blizzard's deltas, never from a copy in the repo.

## For the other agent
- Client data and interface changes go in `tools/clientpatch/edits.json`, never by hand into the client. Claude Code
  installs the archives on deploy.

## Next
- DB2 tables, icons and art when the first edit needs them; the launcher installs the same files.
- GlueXML and FrameXML are signed differently (`GLUEXML.TOC.SIG`) and untested; expect an exe patch for those.
