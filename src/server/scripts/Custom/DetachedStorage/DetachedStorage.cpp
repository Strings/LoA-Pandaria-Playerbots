/*
 * Detached item storage (T-018). See DetachedStorage.h and README.md.
 */

#include "DetachedStorage.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "WorldSession.h"

#include <algorithm>
#include <sstream>

using namespace Detached;

namespace
{
    void ReadEnchants(std::string const& text, std::array<uint32, EnchantSlots>& enchants)
    {
        // item_instance.enchantments holds id, duration and charges per enchantment slot.
        std::istringstream in(text);
        uint32 value = 0;
        for (uint32 index = 0; index < EnchantSlots * 3 && (in >> value); ++index)
            if (index % 3 == 0)
                enchants[index / 3] = value;
    }

    Entry MakeEntry(Item* item, Scope scope, Kind kind, uint32 container, uint32 slot)
    {
        Entry entry;
        entry.ItemGuid = item->GetGUID().GetCounter();
        entry.ItemEntry = item->GetEntry();
        entry.Count = item->GetCount();
        entry.OwnerScope = scope;
        entry.StoreKind = kind;
        entry.Container = container;
        entry.Slot = slot;
        for (uint8 index = 0; index < EnchantSlots; ++index)
            entry.Enchants[index] = item->GetEnchantmentId(EnchantmentSlot(index));
        entry.RandomProperty = item->GetItemRandomPropertyId();
        return entry;
    }
}

DetachedStorage* DetachedStorage::instance()
{
    static DetachedStorage instance;
    return &instance;
}

uint32 DetachedStorage::OwnerId(Player* player, Scope scope)
{
    return scope == Scope::Account ? player->GetSession()->GetAccountId() : player->GetGUID().GetCounter();
}

void DetachedStorage::LoadPlayer(Player* player)
{
    uint32 guid = player->GetGUID().GetCounter();
    std::vector<Entry> entries;
    if (QueryResult result = CharacterDatabase.PQuery("SELECT d.`item_guid`, d.`scope`, d.`kind`, d.`container`, d.`slot`, "
        "ii.`itemEntry`, ii.`count`, ii.`enchantments`, ii.`randomPropertyId` FROM `detached_item` d "
        "JOIN `item_instance` ii ON ii.`guid` = d.`item_guid` WHERE (d.`scope` = 0 AND d.`owner` = %u) OR (d.`scope` = 1 AND d.`owner` = %u)",
        guid, player->GetSession()->GetAccountId()))
    {
        do
        {
            Field* fields = result->Fetch();
            Entry entry;
            entry.ItemGuid = fields[0].GetUInt32();
            entry.OwnerScope = Scope(fields[1].GetUInt8());
            entry.StoreKind = Kind(fields[2].GetUInt8());
            entry.Container = fields[3].GetUInt32();
            entry.Slot = fields[4].GetUInt32();
            entry.ItemEntry = fields[5].GetUInt32();
            entry.Count = fields[6].GetUInt32();
            ReadEnchants(fields[7].GetString(), entry.Enchants);
            entry.RandomProperty = fields[8].GetInt32();
            entries.push_back(entry);
        } while (result->NextRow());
    }

    std::lock_guard<std::mutex> guard(_lock);
    _players[guid] = std::move(entries);
}

void DetachedStorage::UnloadPlayer(Player* player)
{
    std::lock_guard<std::mutex> guard(_lock);
    _players.erase(player->GetGUID().GetCounter());
}

void DetachedStorage::DeletePlayer(uint32 guid)
{
    {
        std::lock_guard<std::mutex> guard(_lock);
        _players.erase(guid);
    }
    CharacterDatabase.PExecute("DELETE FROM `detached_item` WHERE `scope` = 0 AND `owner` = %u", guid);
}

StoreResult DetachedStorage::Store(Player* player, Item* item, Kind kind, uint32 container, uint32 slot,
    CharacterDatabaseTransaction trans, Scope scope)
{
    if (!item || player->GetItemByGuid(item->GetGUID()) != item)
        return StoreResult::NotInInventory;
    if (item->GetTemplate()->Duration || item->GetUInt32Value(ITEM_FIELD_EXPIRATION))
        return StoreResult::Timed;
    if (item->IsNotEmptyBag())
        return StoreResult::NonEmptyBag;
    {
        std::lock_guard<std::mutex> guard(_lock);
        if (_players.find(player->GetGUID().GetCounter()) == _players.end())
            return StoreResult::NotLoaded;
    }
    if (IsTaken(player, kind, container, slot, scope))
        return StoreResult::SlotTaken;

    // The mail path: out of the inventory, the inventory row gone, the item row kept.
    player->MoveItemFromInventory(item->GetBagSlot(), item->GetSlot(), true);
    item->DeleteFromInventoryDB(trans);
    Insert(player, item, kind, container, slot, trans, scope);
    return StoreResult::Ok;
}

void DetachedStorage::Insert(Player* player, Item* item, Kind kind, uint32 container, uint32 slot,
    CharacterDatabaseTransaction trans, Scope scope)
{
    // A new item (never saved) is written whole; an existing one only if it changed. Its row stays either way.
    item->SetOwnerGUID(player->GetGUID());
    item->SaveToDB(trans);
    trans->PAppend("REPLACE INTO `detached_item` (`item_guid`, `scope`, `owner`, `kind`, `container`, `slot`) VALUES (%u, %u, %u, %u, %u, %u)",
        item->GetGUID().GetCounter(), uint32(scope), OwnerId(player, scope), uint32(kind), container, slot);

    Entry entry = MakeEntry(item, scope, kind, container, slot);
    delete item;

    std::lock_guard<std::mutex> guard(_lock);
    _players[player->GetGUID().GetCounter()].push_back(entry);
}

void DetachedStorage::Restore(Player* player, Item* item, Kind kind, uint32 container, uint32 slot,
    CharacterDatabaseTransaction trans, Scope scope)
{
    Insert(player, item, kind, container, slot, trans, scope);
}

Item* DetachedStorage::Take(Player* player, Kind kind, uint32 container, uint32 slot, CharacterDatabaseTransaction trans, Scope scope)
{
    Entry entry;
    {
        std::lock_guard<std::mutex> guard(_lock);
        auto itr = _players.find(player->GetGUID().GetCounter());
        if (itr == _players.end())
            return nullptr;
        auto found = std::find_if(itr->second.begin(), itr->second.end(), [&](Entry const& e)
        {
            return e.OwnerScope == scope && e.StoreKind == kind && e.Container == container && e.Slot == slot;
        });
        if (found == itr->second.end())
            return nullptr;
        entry = *found;
        itr->second.erase(found);
    }

    trans->PAppend("DELETE FROM `detached_item` WHERE `item_guid` = %u", entry.ItemGuid);

    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(entry.ItemEntry);
    // Field order is Item::LoadFromDB's (the mail item query).
    QueryResult result = proto ? CharacterDatabase.PQuery("SELECT `creatorGuid`, `giftCreatorGuid`, `count`, `duration`, `charges`, "
        "`flags`, `enchantments`, `randomPropertyId`, `reforgeId`, `transmogrifyId`, `upgradeID`, `durability`, `playedTime`, `text`, "
        "`pet_species`, `pet_breed`, `pet_quality`, `pet_level` FROM `item_instance` WHERE `guid` = %u", entry.ItemGuid) : QueryResult();
    Item* item = proto ? NewItemOrBag(proto) : nullptr;
    if (!result || !item->LoadFromDB(entry.ItemGuid, player->GetGUID(), result->Fetch(), entry.ItemEntry, player))
    {
        TC_LOG_ERROR("scripts", "DetachedStorage: %s lost stored item %u (entry %u): its item_instance row or template is missing",
            player->GetName().c_str(), entry.ItemGuid, entry.ItemEntry);
        delete item;
        return nullptr;
    }
    item->SetOwnerGUID(player->GetGUID());
    item->SetState(ITEM_UNCHANGED);
    return item;
}

std::vector<Entry> DetachedStorage::List(Player* player, Kind kind, uint32 container, Scope scope)
{
    std::vector<Entry> entries;
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _players.find(player->GetGUID().GetCounter());
    if (itr == _players.end())
        return entries;
    for (Entry const& entry : itr->second)
        if (entry.OwnerScope == scope && entry.StoreKind == kind && entry.Container == container)
            entries.push_back(entry);
    std::sort(entries.begin(), entries.end(), [](Entry const& a, Entry const& b) { return a.Slot < b.Slot; });
    return entries;
}

bool DetachedStorage::IsTaken(Player* player, Kind kind, uint32 container, uint32 slot, Scope scope)
{
    std::lock_guard<std::mutex> guard(_lock);
    auto itr = _players.find(player->GetGUID().GetCounter());
    if (itr == _players.end())
        return false;
    return std::any_of(itr->second.begin(), itr->second.end(), [&](Entry const& e)
    {
        return e.OwnerScope == scope && e.StoreKind == kind && e.Container == container && e.Slot == slot;
    });
}

std::string DetachedStorage::Link(Entry const& entry, uint8 level)
{
    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(entry.ItemEntry);
    std::ostringstream out;
    // Same layout as the transmogrification links this client accepts: enchant, three socket enchants, socket bonus.
    out << "|c" << std::hex << ItemQualityColors[proto ? proto->Quality : 0] << std::dec << "|Hitem:" << entry.ItemEntry << ':'
        << entry.Enchants[PERM_ENCHANTMENT_SLOT] << ':' << entry.Enchants[SOCK_ENCHANTMENT_SLOT] << ':'
        << entry.Enchants[SOCK_ENCHANTMENT_SLOT_2] << ':' << entry.Enchants[SOCK_ENCHANTMENT_SLOT_3] << ':'
        << entry.Enchants[BONUS_ENCHANTMENT_SLOT] << ':' << entry.RandomProperty << ":0:" << uint32(level)
        << "|h[" << (proto ? proto->Name1 : "Unknown item") << "]|h|r";
    return out.str();
}
