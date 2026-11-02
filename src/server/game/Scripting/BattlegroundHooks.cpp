#include "BattlegroundHooks.h"
#include <algorithm>
#include <vector>
namespace
{
    std::vector<BattlegroundHooks::Callback> callbacks;
}
bool BattlegroundHooks::Register(Callback callback)
{
    if (!callback || std::find(callbacks.begin(), callbacks.end(), callback) != callbacks.end())
        return false;
    callbacks.push_back(callback);
    return true;
}
void BattlegroundHooks::Dispatch(Player* player, bool won)
{
    for (Callback callback : callbacks)
        callback(player, won);
}
