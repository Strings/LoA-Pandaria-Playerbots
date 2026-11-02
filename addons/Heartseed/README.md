# Heartseed addon

A panel for the Heartseed (`src/server/scripts/Custom/Heartseed/`). Open with `/hs`, `/heartseed` or the key binding
(Key Bindings > Heartseed; Shift-H by default when that key is free).

A Celestial mandala: the seed in the middle, glowing in the dominant path's colour, with Xuen (west), Niuzao
(north), Yu'lon (east) and Chi-Ji (south) around it. Below: rank against the character's cap, Growth toward the
next rank, the **Celestial Gift** (the level 10 major's active spell, castable from the panel with its cooldown
shown), one node per major tier (chosen, ready to choose, or locked; tooltips explain), essences with their slots,
weapons fed, and whether the character stands at a Celestial shrine. **Choose paths** opens the server's choice
window (`.seed`).

Sounds: spellbook open and close sounds with the panel, a click on the buttons, and the level-up fanfare with a
flash when a new Gift wakes. Custom art goes in `art/` (see `art/README.md`); built-in textures stand in until then.

## Interface

The addon only reads. It sends `.seed sync` and the server answers with whisper addon messages on prefix `Heartseed`,
tab separated, ending with `E`:

| Line | Fields |
|---|---|
| `N` | the character has no Heartseed |
| `S` | effective rank, account rank, rank cap, Growth, Growth for next rank, stage name, dominant path, weapons fed, open essence slots, shrine name (empty when not at one) |
| `M` | one per major tier: tier level, trait name (empty when not chosen), trait path, Gift spell id (0 for none), trait description |
| `X` | one per essence: name, times collected, slot (0 when not slotted) |

The panel refreshes when it opens, on level up, after a gossip or quest window closes, on a spec change, on a zone
change and when a spell is learned. The Gift button is a secure action button: in combat its spell cannot change, so
a new Gift is applied when combat ends.

Tests: `python -m unittest test_heartseed` in this folder (needs `lupa`).
