#include "GameObjectHooks.h"
#include <algorithm>
#include <vector>
namespace
{
    std::vector<GameObjectHooks::HideFilter> hideFilters;
    std::vector<GameObjectHooks::SparkleFilter> sparkleFilters;

    template <typename T>
    bool Add(std::vector<T>& list, T filter)
    {
        if (!filter || std::find(list.begin(), list.end(), filter) != list.end())
            return false;
        list.push_back(filter);
        return true;
    }
}
bool GameObjectHooks::RegisterHideFilter(HideFilter filter) { return Add(hideFilters, filter); }
bool GameObjectHooks::RegisterSparkleFilter(SparkleFilter filter) { return Add(sparkleFilters, filter); }
bool GameObjectHooks::Hides(GameObject const* go, WorldObject const* seer)
{
    for (HideFilter filter : hideFilters)
        if (filter(go, seer))
            return true;
    return false;
}
bool GameObjectHooks::Sparkles(GameObject const* go, WorldObject const* seer)
{
    for (SparkleFilter filter : sparkleFilters)
        if (filter(go, seer))
            return true;
    return false;
}
