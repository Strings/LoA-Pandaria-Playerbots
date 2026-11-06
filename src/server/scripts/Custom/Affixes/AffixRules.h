#ifndef ULTIMATEMOP_AFFIX_RULES_H
#define ULTIMATEMOP_AFFIX_RULES_H
// Server-independent affix rules: rolls and reward math. Shared by rares now, the corruption dial and Brawler
// mutators later (DESIGN.md, Rares and discovery).
#include <algorithm>
#include <cstdint>
#include <vector>

namespace Affixes
{
    // Which systems may roll an affix (affix_template.hosts).
    enum HostMask : uint32_t
    {
        HOST_RARE       = 0x1,
        HOST_CORRUPTION = 0x2,
        HOST_BRAWLER    = 0x4,
        HOST_WILD_RARE  = 0x8
    };

    struct Candidate
    {
        uint32_t Id;
        uint32_t Weight;
    };

    // Picks an index by weight; `uniform(n)` returns a value in [0, n). Returns weights.size() when every weight is 0.
    template <class Uniform>
    std::size_t PickWeighted(std::vector<uint32_t> const& weights, Uniform&& uniform)
    {
        uint64_t total = 0;
        for (uint32_t w : weights)
            total += w;
        if (!total)
            return weights.size();
        uint64_t roll = uniform(total);
        for (std::size_t i = 0; i < weights.size(); ++i)
        {
            if (roll < weights[i])
                return i;
            roll -= weights[i];
        }
        return weights.size() - 1;
    }

    // How many affixes a spawn gets: `countWeights[n]` is the weight of n affixes.
    template <class Uniform>
    uint32_t RollCount(std::vector<uint32_t> const& countWeights, Uniform&& uniform)
    {
        std::size_t index = PickWeighted(countWeights, uniform);
        return index >= countWeights.size() ? 0 : uint32_t(index);
    }

    // Up to `count` distinct affixes, each picked by weight from what is left.
    template <class Uniform>
    std::vector<uint32_t> RollDistinct(std::vector<Candidate> pool, uint32_t count, Uniform&& uniform)
    {
        std::vector<uint32_t> picked;
        while (picked.size() < count && !pool.empty())
        {
            std::vector<uint32_t> weights;
            weights.reserve(pool.size());
            for (Candidate const& c : pool)
                weights.push_back(c.Weight);
            std::size_t index = PickWeighted(weights, uniform);
            if (index >= pool.size())
                break;
            picked.push_back(pool[index].Id);
            pool.erase(pool.begin() + index);
        }
        return picked;
    }

    // Kill XP as a fraction of the killer's current level: a base share plus each affix's bonus, capped.
    inline float RareXpFraction(float base, std::vector<uint32_t> const& affixBonusPct, float max)
    {
        float fraction = base;
        for (uint32_t pct : affixBonusPct)
            fraction += pct / 100.0f;
        return std::clamp(fraction, 0.0f, std::max(0.0f, max));
    }

    // Intensity scales an affix for a lesser or greater host (wild rares run at about 0.55; 1 is full strength).
    // Amounts scale directly; an affix with no amount (Teleporting, Sha-touched) fires less or more often instead.
    inline float ClampIntensity(float intensity)
    {
        return std::clamp(intensity, 0.0f, 10.0f);
    }

    inline float ScaledAmount(float pct, float intensity)
    {
        return pct * ClampIntensity(intensity);
    }

    inline uint32_t ScaledPeriod(uint32_t periodMs, float intensity, bool hasAmount)
    {
        float scale = ClampIntensity(intensity);
        if (hasAmount || !periodMs || scale <= 0.0f)
            return periodMs;
        double value = double(periodMs) / scale;
        return value >= 4294967295.0 ? 0xFFFFFFFFu : std::max<uint32_t>(1, uint32_t(value + 0.5));
    }

    inline uint32_t ScaledCount(uint32_t count, float intensity)
    {
        if (!count)
            return 0;
        return std::max<uint32_t>(1, uint32_t(float(count) * ClampIntensity(intensity) + 0.5f));
    }

    // Damage or shield amounts are a share of a unit's maximum health, so an affix stays a threat at any level.
    inline int32_t ShareOfHealth(uint32_t maxHealth, float pct)
    {
        if (pct <= 0.0f)
            return 0;
        double value = double(maxHealth) * pct / 100.0;
        return value >= 2147483647.0 ? 2147483647 : std::max<int32_t>(1, int32_t(value));
    }
}
#endif
