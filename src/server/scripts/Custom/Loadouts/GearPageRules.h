/*
 * Gear pages (T-018): server-independent rules, kept out of the swap code so they can be unit tested.
 *
 * A loadout's page holds the pieces that differ from what its owner wears in other loadouts. Switching stores the
 * outgoing pieces in the slots the incoming page fills and equips the incoming ones; every other slot stays as it is,
 * so a fresh loadout (an empty page) simply keeps the current gear.
 */

#ifndef ULTIMATEMOP_GEAR_PAGE_RULES_H
#define ULTIMATEMOP_GEAR_PAGE_RULES_H

#include <algorithm>
#include <cstdint>
#include <vector>

namespace GearPages
{
    // Equipment slots (EQUIPMENT_SLOT_*).
    constexpr uint8_t SlotCount = 19;
    constexpr uint8_t MainHand = 15;
    constexpr uint8_t OffHand = 16;
    constexpr uint8_t NoSlot = 255;
    // Page slots from here on hold pieces that could not go back into an equipment slot (bags full, say).
    constexpr uint32_t OverflowStart = 100;

    // Inventory types (INVTYPE_*) and weapon subclasses (ITEM_SUBCLASS_WEAPON_*) the rules need.
    enum InventoryType : uint8_t
    {
        InvHead = 1, InvNeck = 2, InvShoulders = 3, InvBody = 4, InvChest = 5, InvWaist = 6, InvLegs = 7, InvFeet = 8,
        InvWrists = 9, InvHands = 10, InvFinger = 11, InvTrinket = 12, InvWeapon = 13, InvShield = 14, InvRanged = 15,
        InvCloak = 16, InvTwoHand = 17, InvTabard = 19, InvRobe = 20, InvMainHand = 21, InvOffHand = 22, InvHoldable = 23,
        InvThrown = 25, InvRangedRight = 26,
    };
    constexpr uint8_t SubclassPolearm = 6;
    constexpr uint8_t SubclassStaff = 10;
    constexpr uint8_t SubclassWand = 19;

    // The equipment slots an item of this inventory type can sit in on a page, in preference order.
    inline std::vector<uint8_t> SlotsFor(uint8_t inventoryType, bool canDualWield, bool titanGrip)
    {
        switch (inventoryType)
        {
            case InvHead:        return { 0 };
            case InvNeck:        return { 1 };
            case InvShoulders:   return { 2 };
            case InvBody:        return { 3 };
            case InvChest:
            case InvRobe:        return { 4 };
            case InvWaist:       return { 5 };
            case InvLegs:        return { 6 };
            case InvFeet:        return { 7 };
            case InvWrists:      return { 8 };
            case InvHands:       return { 9 };
            case InvFinger:      return { 10, 11 };
            case InvTrinket:     return { 12, 13 };
            case InvCloak:       return { 14 };
            case InvWeapon:      return canDualWield ? std::vector<uint8_t>{ MainHand, OffHand } : std::vector<uint8_t>{ MainHand };
            case InvTwoHand:     return titanGrip ? std::vector<uint8_t>{ MainHand, OffHand } : std::vector<uint8_t>{ MainHand };
            case InvMainHand:
            case InvRanged:
            case InvRangedRight:
            case InvThrown:      return { MainHand };
            case InvShield:
            case InvOffHand:
            case InvHoldable:    return { OffHand };
            case InvTabard:      return { 18 };
            default:             return { };
        }
    }

    // Whether a main-hand item leaves no room for an off hand.
    inline bool BlocksOffHand(uint8_t inventoryType, uint8_t subclass, bool titanGrip)
    {
        if (inventoryType == InvTwoHand)
            return !titanGrip || subclass == SubclassStaff || subclass == SubclassPolearm;
        if (inventoryType == InvRanged || inventoryType == InvRangedRight)
            return subclass != SubclassWand;
        return false;
    }

    // Where a piece put on a page goes: the requested slot if it fits there, else the first fitting free slot, else
    // the first fitting slot (its current piece is swapped out by the caller). NoSlot when it doesn't fit at all.
    inline uint8_t ChooseSlot(std::vector<uint8_t> const& fits, std::vector<uint32_t> const& taken, uint8_t requested)
    {
        if (fits.empty())
            return NoSlot;
        if (requested != NoSlot)
            return std::find(fits.begin(), fits.end(), requested) != fits.end() ? requested : NoSlot;
        for (uint8_t slot : fits)
            if (std::find(taken.begin(), taken.end(), slot) == taken.end())
                return slot;
        return fits.front();
    }

    // The worn pieces to store on the outgoing page: every slot the incoming page fills, plus the off hand when the
    // incoming main hand needs both hands, plus a two-handed main hand when the incoming page brings an off hand.
    inline std::vector<uint8_t> SlotsToStore(std::vector<uint8_t> const& incoming, std::vector<uint8_t> const& worn,
        bool incomingMainBlocksOffHand, bool wornMainBlocksOffHand)
    {
        auto has = [](std::vector<uint8_t> const& slots, uint8_t slot) { return std::find(slots.begin(), slots.end(), slot) != slots.end(); };
        std::vector<uint8_t> store;
        for (uint8_t slot : incoming)
            if (slot < SlotCount && has(worn, slot))
                store.push_back(slot);
        if (has(incoming, MainHand) && incomingMainBlocksOffHand && has(worn, OffHand) && !has(store, OffHand))
            store.push_back(OffHand);
        if (has(incoming, OffHand) && !has(incoming, MainHand) && wornMainBlocksOffHand && has(worn, MainHand) && !has(store, MainHand))
            store.push_back(MainHand);
        std::sort(store.begin(), store.end());
        return store;
    }

    // The first overflow slot nothing uses.
    inline uint32_t FreeOverflow(std::vector<uint32_t> const& taken)
    {
        uint32_t slot = OverflowStart;
        while (std::find(taken.begin(), taken.end(), slot) != taken.end())
            ++slot;
        return slot;
    }
}

#endif
