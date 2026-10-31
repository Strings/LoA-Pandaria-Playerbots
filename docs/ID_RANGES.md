# Reserved ID ranges

Custom content uses reserved ID blocks so agents never collide with each other or with Blizzard
data. Each system gets a block of 1000 per type. Allocate the next free block here (in your PR)
before adding a new system; within a block, the owning task's agent assigns IDs in order.

Stock MoP data stays well below these ranges (quests below ~34000, items below ~110000, spells below
~150000), so the blocks start far above them.

| System | Quests | Creatures | Game objects | Items | Spells | Gossip menus and texts | Tables |
|---|---|---|---|---|---|---|---|
| Heartseed | 90000–90999 | 900000–900999 | 900000–900999 | 190000–190999 | 300000–300999 | 90000–90999 | `heartseed_*` |
| Next system | 91000–91999 | 901000–901999 | 901000–901999 | 191000–191999 | 301000–301999 | 91000–91999 | `<system>_*` |

## Notes

- **Spells:** spells in the 300000 block exist only on the server (`spell_dbc`) until the client
  patch adds matching rows, so they have no client visuals or tooltips yet. Until then, prefer
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
