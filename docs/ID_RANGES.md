# Reserved ID ranges

Custom content uses reserved ID blocks so agents never collide with each other or with Blizzard
data. Each system gets a block of 1000 per type. Allocate the next free block here (in your PR)
before adding a new system; within a block, the owning task's agent assigns IDs in order.

Stock MoP data stays well below these ranges (quests below ~34000, items below ~110000, spells below
~150000), so the blocks start far above them.

| System | Quests | Creatures | Game objects | Items | Spells | Gossip menus and texts | Tables |
|---|---|---|---|---|---|---|---|
| Heartseed | 90000–90999 | 900000–900999 | 900000–900999 | 190000–190999 | 310000–310999 | 90000–90999 | `heartseed_*` |
| GM tools (T-039, T-048) | 91000–91999 | 901000–901999 | 901000–901999 | 191000–191999 | 311000–311999 | 91000–91999 | `gmtools_*` |
| Next system | 92000–92999 | 902000–902999 | 902000–902999 | 192000–192999 | 312000–312999 | 92000–92999 | `<system>_*` |

## Notes

- **Spells:** custom spell blocks start at 310000 because upstream already uses 300000–300109 (and
  scattered ids from 200000 up) in `spell_dbc` for its own server-side spells. Spells in these blocks
  exist only on the server (`spell_dbc`) until the client patch adds matching rows, so they have no
  client visuals or tooltips yet. Until then, prefer
  existing spell IDs for anything the player sees, and use the reserved block for server-side
  helper spells and auras.
- **Items:** new items need hotfix rows so the client shows them correctly (see `docs/DESIGN.md`,
  Collections and Porting).
- **Achievements and anything else defined only in client data** need the client patch; reserve a
  block when that pipeline exists.
- **RBAC permissions** for a system's GM commands use the low end of its gossip block (Heartseed:
  `90000`, `Command: heartseed`), linked to role 197 (Gamemaster Commands) by an update in
  `sql/updates/master/auth/`.
- The SQL validator (T-034) checks new rows against these ranges.
