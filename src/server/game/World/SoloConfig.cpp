#include "SoloConfig.h"
#include "Config.h"
#include <atomic>
#include <cstdint>

namespace
{
    constexpr std::uint8_t WeeklyCaps = 1;
    constexpr std::uint8_t FishingAlways = 2;
    std::atomic<std::uint8_t> settings{ WeeklyCaps };
}

void SoloConfig::Load()
{
    std::uint8_t next = 0;
    if (sConfigMgr->GetBoolDefault("Currency.WeeklyCaps.Enable", true))
        next |= WeeklyCaps;
    if (sConfigMgr->GetBoolDefault("SkillChance.Fishing.Always", false))
        next |= FishingAlways;
    settings.store(next, std::memory_order_relaxed);
}

bool SoloConfig::WeeklyCurrencyCapsEnabled()
{
    return (settings.load(std::memory_order_relaxed) & WeeklyCaps) != 0;
}

bool SoloConfig::GuaranteedFishingSkillups()
{
    return (settings.load(std::memory_order_relaxed) & FishingAlways) != 0;
}
