/*
 * Extra places a player's spell reagents can come from besides the bags (T-043: the reagent bank). The spell code
 * asks here after the bags; a script registers the source at startup. With nothing registered, reagents behave as
 * in the stock core.
 */

#ifndef ULTIMATEMOP_REAGENT_SOURCE_H
#define ULTIMATEMOP_REAGENT_SOURCE_H

#include "Define.h"

class Player;

namespace ReagentSource
{
    // How many of an item the source holds for the player.
    using CountFn = uint32 (*)(Player* player, uint32 itemId);
    // Uses up to count of an item; returns how many it used.
    using TakeFn = uint32 (*)(Player* player, uint32 itemId, uint32 count);

    // Register at script startup only. One source; a second call replaces the first.
    void Register(CountFn count, TakeFn take);

    // Bags plus the source.
    bool HasReagent(Player* player, uint32 itemId, uint32 count);
    // Takes from the bags first, the rest from the source.
    void TakeReagent(Player* player, uint32 itemId, uint32 count);
}

#endif
