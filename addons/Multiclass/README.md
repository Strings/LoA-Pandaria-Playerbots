# Multiclass addon

The second-class panel (T-094, `src/server/scripts/Custom/Multiclass/`). Open with Alt-N (bound on first login when
the key is free; Key Bindings > Multiclass changes it) or `/mc`.

Tabs list every second class your base class can take, with its level. The selected class shows its description,
level and experience bar, a **Make active** / **Set aside** button (`.mc switch <id>` / `.mc switch none`), and its
spellbook: one row per ability with the level that teaches it. Learned abilities of the active class cast on click
and drag to an action bar; locked ones are greyed. Passives are listed with their effect.

## Interface

It sends `.mc sync`; the server answers with whisper addon messages on prefix `Multiclass`, tab separated, ending with
`E`. Level-ups and switches push the same sync unasked.

| Line | Fields |
|---|---|
| `U` | unlock level, character level, experience per second-class level |
| `C` | one per second class: id, name, level, experience, max level, active (0/1), taken (0/1), description |
| `A` | one per ability: class id, level, spell id, kind (0 active, 1 passive), cooldown ms, name, description |

Buttons are secure action buttons, so the panel relayouts only out of combat.
