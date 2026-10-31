#include "MountedSpeed.h"
#include "Config.h"
#include "Log.h"
#include <atomic>

namespace
{
    std::atomic<float> multiplier{ 2.0f };
}

void MountedSpeed::Load()
{
    float configured = sConfigMgr->GetFloatDefault("Rate.MountedSpeed", 2.0f);
    float value = ValidMultiplier(configured);
    if (value != configured)
        TC_LOG_ERROR("server.loading", "Rate.MountedSpeed must be finite and positive; using 2.0.");
    multiplier.store(value, std::memory_order_relaxed);
}

float MountedSpeed::Multiplier()
{
    return multiplier.load(std::memory_order_relaxed);
}
