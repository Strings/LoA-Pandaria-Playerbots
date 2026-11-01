/*
 * Gear pages (T-018). Pieces go on and off a page through the mail path (DetachedStorage) and onto the character
 * through the normal equip path, so unique-equipped, two-hand and dual-wield rules, set bonuses and equip scripts
 * (the Heartseed's) all run as usual. See GearPageRules.h and README.md.
 */

#include "GearPages.h"
#include "GearPageRules.h"
#include "LoadoutMgr.h"
#include "../DetachedStorage/DetachedStorage.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Item.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"

#include <algorithm>

using Detached::Kind;

namespace
{
    std::vector<uint32> PageSlots(Player* player, uint32 loadoutId)
    {
        std::vector<uint32> slots;
        for (Detached::Entry const& entry : sDetachedStorage->List(player, Kind::GearPage, loadoutId))
            slots.push_back(entry.Slot);
        return slots;
    }

    bool MainBlocksOffHand(Player* player, ItemTemplate const* proto)
    {
        return proto && GearPages::BlocksOffHand(uint8(proto->InventoryType), uint8(proto->SubClass), player->CanTitanGrip());
    }

    // Into the bags if there is room, else back onto the page in an overflow slot. Returns true if it reached the bags.
    bool ToBagsOrOverflow(Player* player, Item* item, uint32 loadoutId, CharacterDatabaseTransaction trans)
    {
        ItemPosCountVec dest;
        if (player->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false) == EQUIP_ERR_OK)
        {
            player->MoveItemToInventory(dest, item, true);
            return true;
        }
        sDetachedStorage->Restore(player, item, Kind::GearPage, loadoutId, GearPages::FreeOverflow(PageSlots(player, loadoutId)), trans);
        return false;
    }

    bool KnownLoadout(Player* player, uint32 loadoutId)
    {
        for (Loadouts::Loadout const& loadout : sLoadoutMgr->GetLoadouts(player))
            if (loadout.Id == loadoutId)
                return true;
        return false;
    }
}

bool GearPages::Enabled()
{
    return sLoadoutMgr->Enabled() && sConfigMgr->GetBoolDefault("Loadouts.GearPages.Enable", true);
}

void GearPages::Register()
{
    Loadouts::Listener listener;
    listener.OnSwitched = [](Player* player, uint32 fromId, uint32 toId)
    {
        SwapReport report = Swap(player, fromId, toId);
        if (report.ToBags || report.Overflow)
            TC_LOG_DEBUG("scripts", "Loadouts: %s gear page swap %u -> %u: %u stored, %u equipped, %u to bags, %u left on the page",
                player->GetName().c_str(), fromId, toId, report.Stored, report.Equipped, report.ToBags, report.Overflow);
    };
    // A page with pieces on it can't go: they would have nowhere to live.
    listener.CanDelete = [](Player* player, uint32 loadoutId)
    {
        return sDetachedStorage->List(player, Kind::GearPage, loadoutId).empty();
    };
    sLoadoutMgr->RegisterListener(std::move(listener));
}

GearPages::SwapReport GearPages::Swap(Player* player, uint32 fromId, uint32 toId)
{
    SwapReport report;
    if (!Enabled() || fromId == toId)
        return report;
    std::vector<Detached::Entry> incoming = sDetachedStorage->List(player, Kind::GearPage, toId);
    if (incoming.empty())
        return report;   // an empty page keeps what is worn

    std::vector<uint8> incomingSlots;
    bool incomingMainBlocks = false;
    for (Detached::Entry const& entry : incoming)
    {
        if (entry.Slot >= SlotCount)
            continue;
        incomingSlots.push_back(uint8(entry.Slot));
        if (entry.Slot == MainHand)
            incomingMainBlocks = MainBlocksOffHand(player, sObjectMgr->GetItemTemplate(entry.ItemEntry));
    }
    std::vector<uint8> worn;
    for (uint8 slot = 0; slot < SlotCount; ++slot)
        if (player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            worn.push_back(slot);
    Item* wornMain = player->GetItemByPos(INVENTORY_SLOT_BAG_0, MainHand);
    bool wornMainBlocks = wornMain && MainBlocksOffHand(player, wornMain->GetTemplate());

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();

    // Off first, so the incoming pieces find their slots free.
    for (uint8 slot : SlotsToStore(incomingSlots, worn, incomingMainBlocks, wornMainBlocks))
    {
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        std::vector<uint32> taken = PageSlots(player, fromId);
        uint32 pageSlot = std::find(taken.begin(), taken.end(), uint32(slot)) == taken.end() ? slot : FreeOverflow(taken);
        if (sDetachedStorage->Store(player, item, Kind::GearPage, fromId, pageSlot, trans) == Detached::StoreResult::Ok)
            ++report.Stored;
    }

    // Then on, main hand before off hand. Pieces that can't be worn (a timed piece still in the slot, a level or
    // unique-equipped rule) go to the bags; the overflow pieces of the page are retried the same way.
    for (Detached::Entry const& entry : incoming)
    {
        Item* item = sDetachedStorage->Take(player, Kind::GearPage, toId, entry.Slot, trans);
        if (!item)
            continue;
        uint16 dest = 0;
        if (entry.Slot < SlotCount && player->CanEquipItem(uint8(entry.Slot), dest, item, false, false) == EQUIP_ERR_OK)
        {
            player->EquipItem(dest, item, true);
            item->SetState(ITEM_NEW, player);   // its inventory row was removed when it went on the page
            ++report.Equipped;
        }
        else if (ToBagsOrOverflow(player, item, toId, trans))
            ++report.ToBags;
        else
            ++report.Overflow;
    }

    player->SaveInventoryAndGoldToDB(trans);
    CharacterDatabase.CommitTransaction(trans);
    return report;
}

GearPages::PutResult GearPages::Put(Player* player, uint32 loadoutId, Item* item, uint8 requestedSlot, uint8& slot)
{
    if (!Enabled())
        return PutResult::Disabled;
    if (!KnownLoadout(player, loadoutId))
        return PutResult::UnknownLoadout;
    if (loadoutId == sLoadoutMgr->GetActiveId(player))
        return PutResult::WornPage;
    if (player->IsInCombat())
        return PutResult::InCombat;
    if (!item)
        return PutResult::NotEquippable;

    // Any slot the piece can take in some spec; the swap's equip rules decide for the spec actually switched to.
    std::vector<uint8> fits = SlotsFor(uint8(item->GetTemplate()->InventoryType), true, true);
    slot = ChooseSlot(fits, PageSlots(player, loadoutId), requestedSlot);
    if (slot == NoSlot)
        return PutResult::NotEquippable;

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    Item* previous = sDetachedStorage->Take(player, Kind::GearPage, loadoutId, slot, trans);
    if (sDetachedStorage->Store(player, item, Kind::GearPage, loadoutId, slot, trans) != Detached::StoreResult::Ok)
    {
        if (previous)
            sDetachedStorage->Restore(player, previous, Kind::GearPage, loadoutId, slot, trans);
        CharacterDatabase.CommitTransaction(trans);
        return PutResult::NotStorable;
    }
    if (previous)
        ToBagsOrOverflow(player, previous, loadoutId, trans);
    player->SaveInventoryAndGoldToDB(trans);
    CharacterDatabase.CommitTransaction(trans);
    return PutResult::Ok;
}

GearPages::TakeResult GearPages::Take(Player* player, uint32 loadoutId, uint32 slot)
{
    if (!Enabled())
        return TakeResult::Disabled;
    if (!KnownLoadout(player, loadoutId))
        return TakeResult::UnknownLoadout;
    if (player->IsInCombat())
        return TakeResult::InCombat;
    if (!sDetachedStorage->IsTaken(player, Kind::GearPage, loadoutId, slot))
        return TakeResult::Empty;

    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    Item* item = sDetachedStorage->Take(player, Kind::GearPage, loadoutId, slot, trans);
    if (!item)
    {
        CharacterDatabase.CommitTransaction(trans);
        return TakeResult::Empty;
    }
    ItemPosCountVec dest;
    TakeResult result = TakeResult::Ok;
    if (player->CanStoreItem(NULL_BAG, NULL_SLOT, dest, item, false) == EQUIP_ERR_OK)
        player->MoveItemToInventory(dest, item, true);
    else
    {
        sDetachedStorage->Restore(player, item, Kind::GearPage, loadoutId, slot, trans);
        result = TakeResult::BagsFull;
    }
    player->SaveInventoryAndGoldToDB(trans);
    CharacterDatabase.CommitTransaction(trans);
    return result;
}
