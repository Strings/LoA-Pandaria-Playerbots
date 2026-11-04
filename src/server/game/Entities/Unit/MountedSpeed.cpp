#include "MountedSpeed.h"
#include "Config.h"
#include "Log.h"
#include <atomic>

namespace
{
    std::atomic<float> multiplier{ 2.4f };
    std::atomic<float> ghostSpeed{ 2.0f };
    std::atomic<float> taxiFactor{ 2.0f };
}

void MountedSpeed::Load()
{
    float configured = sConfigMgr->GetFloatDefault("Rate.MountedSpeed", 2.4f);
    float value = ValidMultiplier(configured);
    if (value != configured)
        TC_LOG_ERROR("server.loading", "Rate.MountedSpeed must be finite and positive; using 2.4.");
    multiplier.store(value, std::memory_order_relaxed);

    float ghost = sConfigMgr->GetFloatDefault("Death.GhostSpeed", 2.0f);
    if (!std::isfinite(ghost) || ghost < 0.0f)
    {
        TC_LOG_ERROR("server.loading", "Death.GhostSpeed must be finite and not negative; using 2.0.");
        ghost = 2.0f;
    }
    ghostSpeed.store(ghost, std::memory_order_relaxed);

    float taxi = sConfigMgr->GetFloatDefault("Taxi.SpeedOfTopFlight", 2.0f);
    if (!std::isfinite(taxi) || taxi < 0.0f)
    {
        TC_LOG_ERROR("server.loading", "Taxi.SpeedOfTopFlight must be finite and not negative; using 2.0.");
        taxi = 2.0f;
    }
    taxiFactor.store(taxi, std::memory_order_relaxed);
}

float MountedSpeed::Multiplier()
{
    return multiplier.load(std::memory_order_relaxed);
}

float MountedSpeed::GhostSpeed()
{
    return ghostSpeed.load(std::memory_order_relaxed);
}

float MountedSpeed::TaxiFactor()
{
    return taxiFactor.load(std::memory_order_relaxed);
}
