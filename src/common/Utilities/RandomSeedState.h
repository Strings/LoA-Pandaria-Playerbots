#ifndef ULTIMATEMOP_RANDOM_SEED_STATE_H
#define ULTIMATEMOP_RANDOM_SEED_STATE_H

#include <cstdint>

namespace Trinity
{
    struct RandomSeedState
    {
        bool Enabled = false;
        std::uint32_t Seed = 0;
        std::uint64_t Draws = 0;
    };
}

#endif
