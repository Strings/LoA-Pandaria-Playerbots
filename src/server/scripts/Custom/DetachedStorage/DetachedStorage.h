/*
 * Detached item storage (T-018): items held outside the inventory, the way mail keeps them, for gear pages, the
 * reagent bank (T-043) and twink templates. A stored item keeps its item_instance row (enchants, gems, durability,
 * transmog, binding) and gets a detached_item row instead of a character_inventory row, so it takes no bag space
 * and goes with its owner on delete. See README.md.
 */

#ifndef ULTIMATEMOP_DETACHED_STORAGE_H
#define ULTIMATEMOP_DETACHED_STORAGE_H

#include "DatabaseEnvFwd.h"
#include "Define.h"

#include <array>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Item;
class Player;

namespace Detached
{
    // Who a container belongs to. Twink templates are shared by every character of an account.
    enum class Scope : uint8
    {
        Character = 0,
        Account   = 1,
    };

    // What the container is for. Values are stored in detached_item.kind; never renumber.
    enum class Kind : uint8
    {
        GearPage      = 1,   // container = loadout id, slot = equipment slot
        ReagentBank   = 2,   // T-043
        TwinkTemplate = 3,   // twinks tab
    };

    constexpr uint8 EnchantSlots = 6;   // permanent, temporary, three sockets, socket bonus

    struct Entry
    {
        uint32 ItemGuid = 0;
        uint32 ItemEntry = 0;
        uint32 Count = 0;
        Scope OwnerScope = Scope::Character;
        Kind StoreKind = Kind::GearPage;
        uint32 Container = 0;
        uint32 Slot = 0;
        std::array<uint32, EnchantSlots> Enchants = { };
        int32 RandomProperty = 0;
    };

    enum class StoreResult
    {
        Ok,
        NotInInventory,
        Timed,           // items with a duration expire; they stay in the inventory
        NonEmptyBag,
        SlotTaken,
        NotLoaded,
    };
}

class DetachedStorage
{
public:
    static DetachedStorage* instance();

    void LoadPlayer(Player* player);
    void UnloadPlayer(Player* player);
    // The character's item_instance rows already go with it (owner_guid); this drops the storage rows.
    void DeletePlayer(uint32 guid);

    // Moves an item from the player's inventory into a container slot. The inventory row and the storage row
    // change in the caller's transaction; the Item object is freed.
    Detached::StoreResult Store(Player* player, Item* item, Detached::Kind kind, uint32 container, uint32 slot,
        CharacterDatabaseTransaction trans, Detached::Scope scope = Detached::Scope::Character);

    // Loads a stored item as a loose Item owned by the player and drops its storage row in the caller's transaction.
    // The caller must place it (EquipItem or MoveItemToInventory) or hand it back with Restore in the same tick, and
    // save the inventory in that transaction. Returns nullptr when nothing is stored there.
    Item* Take(Player* player, Detached::Kind kind, uint32 container, uint32 slot, CharacterDatabaseTransaction trans,
        Detached::Scope scope = Detached::Scope::Character);
    // Puts back an item Take returned that could not be placed, into the given slot (which must be free).
    void Restore(Player* player, Item* item, Detached::Kind kind, uint32 container, uint32 slot,
        CharacterDatabaseTransaction trans, Detached::Scope scope = Detached::Scope::Character);

    // Changes a stored stack's size in place, for stack merging and partial use. Count must be at least 1.
    bool SetCount(Player* player, Detached::Kind kind, uint32 container, uint32 slot, uint32 count,
        CharacterDatabaseTransaction trans, Detached::Scope scope = Detached::Scope::Character);
    // Deletes a stored item for good (a reagent used up by crafting).
    bool Destroy(Player* player, Detached::Kind kind, uint32 container, uint32 slot, CharacterDatabaseTransaction trans,
        Detached::Scope scope = Detached::Scope::Character);

    std::vector<Detached::Entry> List(Player* player, Detached::Kind kind, uint32 container,
        Detached::Scope scope = Detached::Scope::Character);
    bool IsTaken(Player* player, Detached::Kind kind, uint32 container, uint32 slot,
        Detached::Scope scope = Detached::Scope::Character);

    // An item link for chat, from the stored fields.
    static std::string Link(Detached::Entry const& entry, uint8 level);

private:
    DetachedStorage() = default;

    void Insert(Player* player, Item* item, Detached::Kind kind, uint32 container, uint32 slot,
        CharacterDatabaseTransaction trans, Detached::Scope scope);
    static uint32 OwnerId(Player* player, Detached::Scope scope);

    std::mutex _lock;
    // Character guid -> its entries, plus the account's shared ones (loaded with each character of the account).
    std::unordered_map<uint32, std::vector<Detached::Entry>> _players;
};

#define sDetachedStorage DetachedStorage::instance()

#endif
