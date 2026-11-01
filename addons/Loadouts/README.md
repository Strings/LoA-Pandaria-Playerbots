# Loadouts addon

Panel for spec loadouts (T-017). `/lo` opens it. Copy this folder to `Interface\AddOns\Loadouts\` in a client copy
(never into `mop-clean\`). No client patch needed.

Each row is a loadout: its name, its spec, and **Switch**, **Rename** and **X** (delete). The active loadout is green.
**Save current as new** stores your current talents, glyphs, bars and Heartseed majors under a new name.

The addon only sends `.loadout` commands; the list comes from the server (`.loadout sync`), sent when the panel opens
and after each change.
