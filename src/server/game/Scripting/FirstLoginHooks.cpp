#include "FirstLoginHooks.h"
#include <algorithm>
#include <vector>
namespace
{
    std::vector<FirstLoginHooks::Callback> callbacks;
}
bool FirstLoginHooks::Register(Callback callback)
{
    if (!callback || std::find(callbacks.begin(), callbacks.end(), callback) != callbacks.end())
        return false;
    callbacks.push_back(callback);
    return true;
}
void FirstLoginHooks::Dispatch(Player* player)
{
    for (Callback callback : callbacks)
        callback(player);
}
