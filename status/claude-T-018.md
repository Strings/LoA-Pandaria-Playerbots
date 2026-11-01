# T-018: Gear pages per loadout

**Agent:** claude
**Branch:** claude/T-018-gear-pages
**Task status:** review

## What changed
- `src/server/scripts/Custom/DetachedStorage/`: the shared detached item storage (gear pages now; reagent bank T-043
  and twink templates later). Items keep their `item_instance` row and get a `detached_item` row instead of an
  inventory row, through the mail path. Character or account scope, kinds 1-3 reserved.
- `src/server/scripts/Custom/Loadouts/`: gear pages. Switching loadouts (by `.loadout switch` or the client's own spec
  switch) stores the worn pieces the incoming page replaces and equips the page through the normal equip path.
  `.loadout page`, `.loadout page put`, `.loadout page take`; `.loadout inspect` shows pages. `LoadoutMgr` gained
  listeners (`OnSwitched`, `CanDelete`) for systems that follow the worn loadout.
- `addons/Loadouts/`: a **Gear** button per loadout opens its page; drag from bags to put, click to take.
- Config `Loadouts.GearPages.Enable` (1). Table `detached_item` (characters).

## How it was tested
- Unit tests: `tests/loadouts/gear_page_rules_test.cpp` (slots per inventory type, two-hand rules, slot choice,
  swap planning, spares); 13/13 suites pass.
- Live (headless, Seedtest, level 59 warrior): put an axe and a buckler on the Protection page (and the refusal for
  the worn loadout); switch to Protection and back with `.loadout switch` (the two-handed club and the axe plus
  buckler trade places, off hand included); the same through the spec-activation spells (63644/63645), i.e. the
  client's own spec switch; take back to bags, empty-slot refusal, delete refusal; pages survive relog; `.loadout sync`
  sends the `G` lines. After every step `character_inventory` and `detached_item` held each piece exactly once.
- Not tested: the addon panel in a real client (needs a playtest; listed in `docs/PLAYTEST.md`), and the
  bags-full path (pieces stay on the page as spares).

## Decisions made
- **A page holds only what differs.** Slots its page leaves empty keep what is worn, so a new loadout starts wearing
  the current gear and nothing is ever stripped. DESIGN.md says "detaches the equipped set"; detaching only the
  slots the incoming page fills gives the same result for full pages and avoids naked swaps for empty ones.
- **The worn loadout's page is what you wear** (nothing is stored on it); you put pieces on other loadouts' pages.
- **A loadout whose page holds pieces can't be deleted** (take them first), rather than mailing them back.
- **Pieces that can't be worn go to the bags; with full bags they stay on the page as spares (slot 100+).**
- **Item links carry socket enchant ids** as the transmog links do; gem icons in the addon tooltip need a playtest look.

## For the other agents
- Apply `sql/updates/characters/2026_11_01_claude_02_detached_storage.sql`.
- T-043 (reagent bank) and the twinks tab use `sDetachedStorage` with their own `Detached::Kind`; read the notes in
  `DetachedStorage/README.md` (stacking, account-scope owners).

## Next
- T-043 reagent bank on the same storage.
- Transmog per page can follow later (DESIGN.md "transmog can follow per page"); items keep their own transmog now.

## Map additions
- `src/server/scripts/Custom/DetachedStorage/`: detached item storage (gear pages, reagent bank, twink templates)
- `tests/loadouts/gear_page_rules_test.cpp`: gear page rule tests
