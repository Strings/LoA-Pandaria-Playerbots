#include "CreatureHooks.h"
#include <algorithm>
#include <vector>
namespace
{
    std::vector<CreatureHooks::Callback> callbacks;
    std::vector<CreatureHooks::FightFilter> filters;
    std::vector<CreatureHooks::VignetteResolver> resolvers;
}
bool CreatureHooks::Register(Callback callback)
{
    if (!callback || std::find(callbacks.begin(), callbacks.end(), callback) != callbacks.end())
        return false;
    callbacks.push_back(callback);
    return true;
}
bool CreatureHooks::RegisterFightFilter(FightFilter filter)
{
    if (!filter || std::find(filters.begin(), filters.end(), filter) != filters.end())
        return false;
    filters.push_back(filter);
    return true;
}
void CreatureHooks::Dispatch(Creature* creature, Event event)
{
    for (Callback callback : callbacks)
        callback(creature, event);
}
bool CreatureHooks::BlocksFight(Creature const* creature, Unit const* other)
{
    for (FightFilter filter : filters)
        if (filter(creature, other))
            return true;
    return false;
}
bool CreatureHooks::RegisterVignetteResolver(VignetteResolver resolver)
{
    if (!resolver || std::find(resolvers.begin(), resolvers.end(), resolver) != resolvers.end())
        return false;
    resolvers.push_back(resolver);
    return true;
}
uint32_t CreatureHooks::VignetteFor(Creature const* creature)
{
    for (VignetteResolver resolver : resolvers)
        if (uint32_t id = resolver(creature))
            return id;
    return 0;
}
