/*
 * Extra reagent sources (T-043). See ReagentSource.h.
 */

#include "ReagentSource.h"
#include "Player.h"

#include <algorithm>

namespace
{
    ReagentSource::CountFn countFn = nullptr;
    ReagentSource::TakeFn takeFn = nullptr;
}

void ReagentSource::Register(CountFn count, TakeFn take)
{
    countFn = count;
    takeFn = take;
}

bool ReagentSource::HasReagent(Player* player, uint32 itemId, uint32 count)
{
    if (player->HasItemCount(itemId, count))
        return true;
    return countFn && player->GetItemCount(itemId) + countFn(player, itemId) >= count;
}

void ReagentSource::TakeReagent(Player* player, uint32 itemId, uint32 count)
{
    uint32 fromBags = std::min(count, player->GetItemCount(itemId));
    if (fromBags)
        player->DestroyItemCount(itemId, fromBags, true);
    if (count > fromBags && takeFn)
        takeFn(player, itemId, count - fromBags);
}
