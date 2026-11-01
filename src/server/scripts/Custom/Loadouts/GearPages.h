/*
 * Gear pages (T-018): each loadout keeps a page of equipment in detached storage. See GearPageRules.h for the
 * swap rules and README.md for the commands.
 */

#ifndef ULTIMATEMOP_GEAR_PAGES_H
#define ULTIMATEMOP_GEAR_PAGES_H

#include "Define.h"

#include <string>
#include <vector>

class Item;
class Player;

namespace GearPages
{
    struct SwapReport
    {
        uint32 Stored = 0;
        uint32 Equipped = 0;
        uint32 ToBags = 0;                      // pieces that could not be equipped and went to the bags
        uint32 Overflow = 0;                    // pieces that fit nowhere and stayed on the page
    };

    enum class PutResult
    {
        Ok,
        Disabled,
        UnknownLoadout,
        WornPage,          // the worn loadout's page is what you wear: equip the piece instead
        NotEquippable,     // not a piece of equipment, or not in that slot
        NotStorable,       // timed item or a bag with items in it
        InCombat,
    };

    enum class TakeResult
    {
        Ok,
        Disabled,
        UnknownLoadout,
        Empty,
        BagsFull,
        InCombat,
    };

    bool Enabled();
    void Register();   // hooks gear pages into the loadout manager

    SwapReport Swap(Player* player, uint32 fromId, uint32 toId);
    // Puts a piece from the bags onto a loadout's page; requestedSlot is an equipment slot or 255 for the first that fits.
    // A piece already in that page slot comes back to the bags in its place.
    PutResult Put(Player* player, uint32 loadoutId, Item* item, uint8 requestedSlot, uint8& slot);
    TakeResult Take(Player* player, uint32 loadoutId, uint32 slot);
}

#endif
