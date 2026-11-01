/*
 * Reagent bank (T-043). Stacks live in detached storage (kind ReagentBank, container 0, one slot per stack); see
 * ReagentBankRules.h for how stacks merge and drain.
 */

#include "ReagentBank.h"
#include "ReagentBankRules.h"
#include "../DetachedStorage/DetachedStorage.h"
#include "Config.h"
#include "DatabaseEnv.h"
#include "Bag.h"
#include "Item.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "ReagentSource.h"
#include "SpellInfo.h"
#include "SpellMgr.h"

#include <map>
#include <unordered_set>

using Detached::Kind;

namespace
{
    constexpr uint32 Container = 0;

    std::unordered_set<uint32> reagents;

    std::vector<ReagentBank::Stack> StacksOf(Player* player, uint32 itemEntry)
    {
        std::vector<ReagentBank::Stack> stacks;
        for (Detached::Entry const& entry : sDetachedStorage->List(player, Kind::ReagentBank, Container))
            if (entry.ItemEntry == itemEntry)
                stacks.push_back({ entry.Slot, entry.Count });
        return stacks;
    }

    std::vector<uint32> TakenSlots(Player* player)
    {
        std::vector<uint32> taken;
        for (Detached::Entry const& entry : sDetachedStorage->List(player, Kind::ReagentBank, Container))
            taken.push_back(entry.Slot);
        return taken;
    }

    uint32 MaxStacks()
    {
        return uint32(std::max(0, sConfigMgr->GetIntDefault("ReagentBank.MaxStacks", 0)));
    }

    // Tops up the bank's stacks of an item with up to count; returns how many found room.
    uint32 Merge(Player* player, uint32 itemEntry, uint32 maxStack, uint32 count, CharacterDatabaseTransaction trans)
    {
        std::vector<ReagentBank::Stack> stacks = StacksOf(player, itemEntry);
        uint32 remainder = 0;
        uint32 merged = 0;
        for (ReagentBank::Move const& move : ReagentBank::PlanDeposit(stacks, maxStack, count, remainder))
        {
            auto stack = std::find_if(stacks.begin(), stacks.end(), [&](ReagentBank::Stack const& s) { return s.Slot == move.Slot; });
            if (sDetachedStorage->SetCount(player, Kind::ReagentBank, Container, move.Slot, stack->Count + move.Amount, trans))
                merged += move.Amount;
        }
        return merged;
    }

    uint32 SourceCount(Player* player, uint32 itemId)
    {
        return ReagentBank::Enabled() ? ReagentBank::Count(player, itemId) : 0;
    }

    uint32 SourceTake(Player* player, uint32 itemId, uint32 count)
    {
        return ReagentBank::Enabled() ? ReagentBank::Use(player, itemId, count) : 0;
    }
}

bool ReagentBank::Enabled()
{
    return sConfigMgr->GetBoolDefault("ReagentBank.Enable", true);
}

void ReagentBank::Register()
{
    reagents.clear();
    for (uint32 spellId = 0; spellId < sSpellMgr->GetSpellInfoStoreSize(); ++spellId)
        if (SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(spellId))
            for (uint32 index = 0; index < MAX_SPELL_REAGENTS; ++index)
                if (spellInfo->Reagent[index] > 0)
                    if (ItemTemplate const* proto = sObjectMgr->GetItemTemplate(uint32(spellInfo->Reagent[index])))
                        if (proto->Class != ITEM_CLASS_QUEST && proto->Bonding != BIND_QUEST && proto->MaxCount <= 0 && !proto->Duration)
                            reagents.insert(proto->ItemId);
    ReagentSource::Register(&SourceCount, &SourceTake);
    TC_LOG_INFO("server.loading", ">> Reagent bank: %u items count as reagents", uint32(reagents.size()));
}

bool ReagentBank::IsReagent(uint32 itemEntry)
{
    return reagents.count(itemEntry) != 0;
}

ReagentBank::DepositResult ReagentBank::Deposit(Player* player, Item* item, uint32& deposited)
{
    deposited = 0;
    if (!Enabled())
        return DepositResult::Disabled;
    if (player->IsInCombat())
        return DepositResult::InCombat;
    if (!item || player->GetItemByGuid(item->GetGUID()) != item || !Player::IsInventoryPos(item->GetBagSlot(), item->GetSlot())
        || Player::IsEquipmentPos(item->GetBagSlot(), item->GetSlot()) || Player::IsBagPos(item->GetPos()))
        return DepositResult::NotInBags;
    if (!IsReagent(item->GetEntry()))
        return DepositResult::NotReagent;
    if (item->GetUInt32Value(ITEM_FIELD_EXPIRATION))
        return DepositResult::NotStorable;

    uint32 count = item->GetCount();
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    deposited = Merge(player, item->GetEntry(), item->GetMaxStackCount(), count, trans);
    DepositResult result = DepositResult::Ok;
    if (deposited == count)
        player->DestroyItem(item->GetBagSlot(), item->GetSlot(), true);
    else
    {
        if (deposited)
        {
            item->SetCount(count - deposited);
            item->SetState(ITEM_CHANGED, player);
            item->SendUpdateToPlayer(player);
        }
        if (!HasRoom(uint32(TakenSlots(player).size()), MaxStacks()))
            result = deposited ? DepositResult::Partial : DepositResult::Full;
        else
        {
            uint32 rest = item->GetCount();
            if (sDetachedStorage->Store(player, item, Kind::ReagentBank, Container, FreeSlot(TakenSlots(player)), trans) == Detached::StoreResult::Ok)
                deposited += rest;
            else if (!deposited)
                result = DepositResult::NotStorable;
        }
    }
    player->SaveInventoryAndGoldToDB(trans);
    CharacterDatabase.CommitTransaction(trans);
    return result;
}

uint32 ReagentBank::DepositAll(Player* player, uint32& stacks, bool& full)
{
    stacks = 0;
    full = false;
    std::vector<std::pair<uint8, uint8>> positions;
    for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
        if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
            if (IsReagent(item->GetEntry()))
                positions.emplace_back(INVENTORY_SLOT_BAG_0, slot);
    for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
        if (Bag* container = player->GetBagByPos(bag))
            for (uint32 slot = 0; slot < container->GetBagSize(); ++slot)
                if (Item* item = container->GetItemByPos(uint8(slot)))
                    if (IsReagent(item->GetEntry()))
                        positions.emplace_back(bag, uint8(slot));

    uint32 total = 0;
    for (auto const& position : positions)
    {
        uint32 deposited = 0;
        DepositResult result = Deposit(player, player->GetItemByPos(position.first, position.second), deposited);
        total += deposited;
        if (deposited)
            ++stacks;
        if (result == DepositResult::Full || result == DepositResult::Partial)
            full = true;
        else if (result != DepositResult::Ok && result != DepositResult::NotStorable && result != DepositResult::NotReagent)
            break;   // disabled or in combat: the rest would fail the same way
    }
    return total;
}

ReagentBank::WithdrawResult ReagentBank::Withdraw(Player* player, uint32 itemEntry, uint32 count, uint32& withdrawn)
{
    withdrawn = 0;
    if (!Enabled())
        return WithdrawResult::Disabled;
    if (player->IsInCombat())
        return WithdrawResult::InCombat;
    count = std::min(count, Count(player, itemEntry));
    if (!count)
        return WithdrawResult::Empty;

    // New items rather than the stored ones, so the bags merge them like any loot.
    ItemPosCountVec dest;
    uint32 noSpace = 0;
    InventoryResult canStore = player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemEntry, count, &noSpace);
    if (canStore != EQUIP_ERR_OK)
    {
        if (noSpace >= count)
            return WithdrawResult::BagsFull;
        count -= noSpace;
        dest.clear();
        if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemEntry, count) != EQUIP_ERR_OK)
            return WithdrawResult::BagsFull;
    }
    withdrawn = Use(player, itemEntry, count);
    if (withdrawn != count)
    {
        // Use takes all or nothing it can find; this only happens if the cache and the plan disagree.
        TC_LOG_ERROR("scripts", "ReagentBank: %s withdrew %u of %u x%u", player->GetName().c_str(), withdrawn, itemEntry, count);
        dest.clear();
        if (!withdrawn || player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemEntry, withdrawn) != EQUIP_ERR_OK)
            return WithdrawResult::Empty;
    }
    if (Item* item = player->StoreNewItem(dest, itemEntry, true))
        player->SendNewItem(item, withdrawn, true, false);
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    player->SaveInventoryAndGoldToDB(trans);
    CharacterDatabase.CommitTransaction(trans);
    return noSpace ? WithdrawResult::Partial : WithdrawResult::Ok;
}

uint32 ReagentBank::Count(Player* player, uint32 itemEntry)
{
    return Total(StacksOf(player, itemEntry));
}

uint32 ReagentBank::Use(Player* player, uint32 itemEntry, uint32 count)
{
    std::vector<Stack> stacks = StacksOf(player, itemEntry);
    uint32 missing = 0;
    std::vector<Move> moves = PlanWithdraw(stacks, count, missing);
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    uint32 used = 0;
    for (Move const& move : moves)
    {
        auto stack = std::find_if(stacks.begin(), stacks.end(), [&](Stack const& s) { return s.Slot == move.Slot; });
        bool ok = move.Amount >= stack->Count
            ? sDetachedStorage->Destroy(player, Kind::ReagentBank, Container, move.Slot, trans)
            : sDetachedStorage->SetCount(player, Kind::ReagentBank, Container, move.Slot, stack->Count - move.Amount, trans);
        if (ok)
            used += move.Amount;
    }
    CharacterDatabase.CommitTransaction(trans);
    return used;
}

uint32 ReagentBank::Add(Player* player, uint32 itemEntry, uint32 count)
{
    ItemTemplate const* proto = sObjectMgr->GetItemTemplate(itemEntry);
    if (!proto || !count)
        return 0;
    uint32 maxStack = std::max<uint32>(1, proto->GetMaxStackSize());
    CharacterDatabaseTransaction trans = CharacterDatabase.BeginTransaction();
    uint32 added = Merge(player, itemEntry, maxStack, count, trans);
    while (added < count)
    {
        Item* item = Item::CreateItem(itemEntry, std::min(maxStack, count - added), player);
        if (!item)
            break;
        uint32 created = item->GetCount();
        sDetachedStorage->Restore(player, item, Kind::ReagentBank, Container, FreeSlot(TakenSlots(player)), trans);
        added += created;
    }
    CharacterDatabase.CommitTransaction(trans);
    return added;
}

std::vector<ReagentBank::Holding> ReagentBank::Contents(Player* player)
{
    std::map<uint32, Holding> byEntry;
    for (Detached::Entry const& entry : sDetachedStorage->List(player, Kind::ReagentBank, Container))
    {
        Holding& holding = byEntry[entry.ItemEntry];
        holding.ItemEntry = entry.ItemEntry;
        holding.Count += entry.Count;
        ++holding.Stacks;
    }
    std::vector<Holding> contents;
    for (auto const& pair : byEntry)
        contents.push_back(pair.second);
    return contents;
}
