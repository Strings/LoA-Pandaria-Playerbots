# Heartseed addon

A panel for the Heartseed (`src/server/scripts/Custom/Heartseed/`). Open with `/hs`, `/heartseed` or the key binding
(Key Bindings > Heartseed; Shift-H by default when that key is free).

Shows the stage and dominant path, rank against the character's cap, Growth toward the next rank, the major chosen at
each tier of the current spec (or "choose one" / "locked"), collected essences with their slots, weapons fed, and
whether the character stands at a Celestial shrine. **Choose paths** opens the server's choice window (`.seed`).

## Interface

The addon only reads. It sends `.seed sync` and the server answers with whisper addon messages on prefix `Heartseed`,
tab separated, ending with `E`:

| Line | Fields |
|---|---|
| `N` | the character has no Heartseed |
| `S` | effective rank, account rank, rank cap, Growth, Growth for next rank, stage name, dominant path, weapons fed, open essence slots, shrine name (empty when not at one) |
| `M` | one per major tier: tier level, trait name (empty when not chosen), trait path |
| `X` | one per essence: name, times collected, slot (0 when not slotted) |

The panel refreshes when it opens, on level up, after a gossip or quest window closes, on a spec change and on a zone
change.
