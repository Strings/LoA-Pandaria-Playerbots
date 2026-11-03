# T-085: Client patch archive

**Agent:** claude
**Branch:** claude/T-085-client-patch
**Task status:** review (waiting on a real-client check)

## What changed
- `tools/clientpatch/`: `edits.json` lists every client edit (what it replaces, the new value, why), `build.py` builds
  them into `wow-update-base-18414.MPQ` and installs or removes it in a client's `Data/`, `mpq.py` writes the
  archive, and `client.py` reads the client's own archives, including Blizzard's PTCH/BSD0 delta patches, to get the
  current version of a file before editing it. No dependencies.
- First edit: the mount journal's `PLAYER_MOUNT_LEVEL` drops from 20 to 1. Below that hard-coded level the journal
  greyed out every mount and refused drags to the bar, whatever the character could ride; the server lets everyone
  ride from 1 (T-011).
- The MountBar addon (PR 122) is removed from the repo and the client; the patch replaces it.
- AGENTS.md: the client folder may now hold our patch archive in `Data/` (the human approved).

## How it was tested
- Unit tests 14/14: archive round trip and the format's reference hash, DBC and text edits, Blizzard delta patches
  and the refusals.
- The current journal file rebuilt from the client's archives through four Blizzard deltas, every MD5 check passing;
  the patched copy differs from it in that one line.
- Real client: the archive loads (no crash). Two dead ends on the way: an uncompressed single-unit file crashed the
  client ("Reached the end of the file"), so files now use Blizzard's plain-sector layout; and lowering Apprentice
  Riding's spell level changed nothing, because the journal checks the character's level in Lua.
- Waiting on the human: mounts no longer grey and drag to the bar.

## Decisions made
- One archive for every client change, named after the client build so it loads after Blizzard's last update.
- Edits name what they replace, so a source change fails the build instead of patching blindly.
- Text edits start from the client's own current file, rebuilt through Blizzard's deltas, never from a copy in the repo.

## For the other agent
- Client data changes go in `tools/clientpatch/edits.json`, never by hand into the client. Claude Code installs
  the archive on deploy.

## Next
- If the client ignores the archive, try another load slot (`Data/enUS/wow-update-enUS-18414.MPQ`).
- DB2 tables, icons and interface files when the first edit needs them; the launcher installs the same file.
