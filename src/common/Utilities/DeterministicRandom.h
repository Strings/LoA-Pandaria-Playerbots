#ifndef ULTIMATEMOP_DETERMINISTIC_RANDOM_H
#define ULTIMATEMOP_DETERMINISTIC_RANDOM_H

#include "RandomSeedState.h"
#include <atomic>
#include <mutex>
#include <random>

namespace Trinity
{
    class DeterministicRandom
    {
    public:
        void Seed(std::uint32_t seed)
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _engine.seed(seed);
            _state = { true, seed, 0 };
            _enabled.store(true, std::memory_order_release);
        }

        void Disable()
        {
            std::lock_guard<std::mutex> lock(_mutex);
            _state.Enabled = false;
            _enabled.store(false, std::memory_order_release);
        }

        bool TryNext(std::uint32_t& value)
        {
            // Production retains its thread-local fast path without a mutex.
            if (!_enabled.load(std::memory_order_acquire))
                return false;
            std::lock_guard<std::mutex> lock(_mutex);
            if (!_state.Enabled)
                return false;
            value = _engine();
            ++_state.Draws;
            return true;
        }

        RandomSeedState Inspect() const
        {
            std::lock_guard<std::mutex> lock(_mutex);
            return _state;
        }

    private:
        std::atomic<bool> _enabled{ false };
        mutable std::mutex _mutex;
        std::mt19937 _engine;
        RandomSeedState _state;
    };
}

#endif
