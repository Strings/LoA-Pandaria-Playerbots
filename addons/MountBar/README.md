# MountBar (MoP 5.4.8)

The server lets every character ride from level 1 (T-011), but the 5.4.8 mount journal greys out mounts below
their spell level ("You do not meet the requirements") and refuses to drag them. This addon re-enables dragging
for those rows only: dragging one puts a summon macro on the cursor, named after the mount, with its icon. The
macro looks the mount up by spell ID when clicked, so it survives new mounts shifting the journal order, and it
dismounts when already mounted. Usable mounts drag as normal companion actions.

Macros go in the character's macro slots, then the shared ones when those are full. They can't be made in
combat. This is the interim fix until the client patch lowers the mount spell levels.

Tests run the Lua in a Lua VM with mocked WoW frames:
`python -m unittest discover -s addons/MountBar -p test_*.py` (requires lupa).
