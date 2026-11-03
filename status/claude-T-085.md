# T-085: Client patch archive

**Agent:** claude
**Branch:** claude/T-085-client-patch
**Task status:** review (waiting on a real-client check)

## What changed
- `tools/clientpatch/`: `edits.json` lists every client data edit (table, row, field, old and new value, why),
  `build.py` builds them into `wow-update-base-18414.MPQ` and installs or removes it in a client's `Data/`, and
  `mpq.py` writes the archive (format 1, uncompressed single-unit files, no dependency).
- First edit: Apprentice Riding (`SpellLevels.dbc` row 6758) drops from level 20 to 1. The server already teaches
  riding at level 1 (T-011), but the client greyed out every ground mount below 20, so they couldn't be dragged
  to the bar ("You do not meet the requirements").
- The MountBar addon (PR 122) is removed from the repo and the client; the patch replaces it.
- AGENTS.md: the client folder may now hold our patch archive in `Data/` (the human approved).

## How it was tested
- Unit tests 7/7: archive round trip, the format's reference hash, DBC cell edits and refusals.
- The built archive reads back with our reader: one byte differs from the source table. The first install crashed the client ("Reached the end of the file" in SpellLevels.dbc): it stored the file as an uncompressed single unit. Blizzard's own update archives store uncompressed files as plain sectors, so the archive now does the same.
- Not yet confirmed in the real client.

## Decisions made
- One archive for every client change, named after the client build so it loads after Blizzard's last update.
- Edits name the value they replace, so a source data change fails the build instead of patching blindly.

## For the other agent
- Client data changes go in `tools/clientpatch/edits.json`, never by hand into the client. Claude Code installs
  the archive on deploy.

## Next
- If the client ignores the archive, try another load slot (`Data/enUS/wow-update-enUS-18414.MPQ`).
- DB2 tables, icons and interface files when the first edit needs them; the launcher installs the same file.
