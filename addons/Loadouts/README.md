# Loadouts addon

Panel for spec loadouts (T-017) and their gear pages (T-018). `/lo` opens it. Copy this folder to `Interface\AddOns\Loadouts\` in a client copy
(never into `mop-clean\`). No client patch needed.

Each row is a loadout: its name, its spec, and **Switch**, **Rename** and **X** (delete). The active loadout is green.
**Save current as new** stores your current talents, glyphs, bars and Heartseed majors under a new name.

**Gear** opens that loadout's gear page beside the panel: its 19 equipment slots, then any spares. Drag a piece from
your bags onto a slot to put it on the page; click a piece to take it back to your bags. Pieces on a page are worn
when you switch to that loadout; empty slots keep what you have on. The loadout you are wearing has nothing on its
page: equip pieces normally.

The addon only sends `.loadout` commands; the list comes from the server (`.loadout sync`), sent when the panel opens
and after each change.
