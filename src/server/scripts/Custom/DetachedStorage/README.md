# Detached item storage (T-018)

Items held outside the inventory, the way mail holds them: they keep their `item_instance` row (enchants, gems,
durability, transmog, binding), lose their `character_inventory` row and get a `detached_item` row instead. They take
no bag space, persist normally and go with the character on delete (`item_instance` by `owner_guid`, our rows by the
delete hook). Built once for gear pages (T-018), the reagent bank (T-043) and twink templates.

## Interface

`DetachedStorage` (`sDetachedStorage`):

- `Store(player, item, kind, container, slot, trans[, scope])`: moves an inventory item into a container slot (the
  mail path: `MoveItemFromInventory`, `DeleteFromInventoryDB`, `SaveToDB`). Refuses timed items, bags with items in
  them and a taken slot.
- `Take(player, kind, container, slot, trans[, scope])`: loads the stored item as a loose `Item` owned by the player and
  drops its storage row. The caller places it in the same tick (`EquipItem` then `SetState(ITEM_NEW)`, or
  `MoveItemToInventory`) or hands it back with `Restore`, and saves the inventory in the same transaction
  (`SaveInventoryAndGoldToDB(trans)`), so a crash can never leave an item in neither place.
- `List`, `IsTaken`, and `Link(entry, level)` for chat links built from the stored fields.

`Kind` (stored in `detached_item.kind`, never renumber): 1 gear page (container = loadout id, slot = equipment slot,
100+ for spares), 2 reagent bank, 3 twink template. `Scope`: character (owner = character guid) or account
(owner = account id; loaded with every character of the account).

Each character's entries are cached at login (`detached_scripts.cpp`); bots load nothing.

## Notes for later users

- The reagent bank needs stacking: merge counts into an existing stored stack before taking a new slot.
- Account-scope items still have a character as `item_instance.owner_guid`. Deleting that character deletes them, so
  twink templates must move the owner (or refuse the delete) when they land.
- `.pdump` and character transfers don't carry stored items.

## Table (characters)

`detached_item`, created by `sql/updates/characters/2026_11_01_claude_02_detached_storage.sql`.
