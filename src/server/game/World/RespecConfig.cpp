#include "RespecConfig.h"
#include "Config.h"
#include <atomic>
namespace
{
    std::atomic<unsigned> settings{ 3 };
}
void RespecConfig::Load()
{
    unsigned next = 0;
    if (sConfigMgr->GetBoolDefault("Respec.FreeReagents", true))
        next |= 1;
    if (sConfigMgr->GetBoolDefault("Respec.AutoDualSpec", true))
        next |= 2;
    settings.store(next, std::memory_order_relaxed);
}
bool RespecConfig::FreeReagents()
{
    return (settings.load(std::memory_order_relaxed) & 1) != 0;
}
bool RespecConfig::AutoDualSpec()
{
    return (settings.load(std::memory_order_relaxed) & 2) != 0;
}
