#include "AffixRules.h"
#include "TestMain.h"
#include <cmath>
#include <set>
void RunTests()
{
    using namespace Affixes;
    // A scripted "random" source: returns the queued values in order (modulo n).
    std::vector<uint64_t> queue;
    std::size_t next = 0;
    auto scripted = [&](uint64_t n) { uint64_t v = queue[next++ % queue.size()]; return v % n; };

    // Weighted pick walks the cumulative weights.
    queue = { 0, 29, 30, 69, 70, 99 }; next = 0;
    std::vector<uint32_t> counts = { 30, 40, 30 };
    UMP_CHECK(PickWeighted(counts, scripted) == 0);
    UMP_CHECK(PickWeighted(counts, scripted) == 0);
    UMP_CHECK(PickWeighted(counts, scripted) == 1);
    UMP_CHECK(PickWeighted(counts, scripted) == 1);
    UMP_CHECK(PickWeighted(counts, scripted) == 2);
    UMP_CHECK(PickWeighted(counts, scripted) == 2);
    UMP_CHECK(PickWeighted(std::vector<uint32_t>{ 0, 0 }, scripted) == 2);   // all zero: nothing
    UMP_CHECK(RollCount(std::vector<uint32_t>{ 0, 0 }, scripted) == 0);

    // Distinct picks never repeat and stop when the pool runs out.
    queue = { 0 }; next = 0;
    std::vector<Candidate> pool = { { 1, 10 }, { 2, 10 }, { 3, 10 } };
    auto picked = RollDistinct(pool, 5, scripted);
    UMP_CHECK(picked.size() == 3);
    UMP_CHECK(std::set<uint32_t>(picked.begin(), picked.end()).size() == 3);
    // Zero-weight affixes are never picked.
    auto none = RollDistinct(std::vector<Candidate>{ { 7, 0 } }, 2, scripted);
    UMP_CHECK(none.empty());

    // XP: base plus bonuses, capped.
    UMP_CHECK(std::fabs(RareXpFraction(0.25f, {}, 1.0f) - 0.25f) < 1e-6f);
    UMP_CHECK(std::fabs(RareXpFraction(0.25f, { 25, 25 }, 1.0f) - 0.75f) < 1e-6f);
    UMP_CHECK(std::fabs(RareXpFraction(0.25f, { 50, 50, 50 }, 1.0f) - 1.0f) < 1e-6f);
    UMP_CHECK(RareXpFraction(0.25f, {}, -1.0f) == 0.0f);

    // Amounts as a share of health; never zero for a positive share, never overflowing.
    UMP_CHECK(ShareOfHealth(1000, 6.0f) == 60);
    UMP_CHECK(ShareOfHealth(10, 1.0f) == 1);
    UMP_CHECK(ShareOfHealth(1000, 0.0f) == 0);
    UMP_CHECK(ShareOfHealth(4000000000u, 100.0f) == 2147483647);

    // Intensity: amounts scale; amount-less affixes fire less often instead; counts never drop to zero.
    UMP_CHECK(std::fabs(ScaledAmount(6.0f, 0.5f) - 3.0f) < 1e-6f);
    UMP_CHECK(ScaledAmount(6.0f, -1.0f) == 0.0f);
    UMP_CHECK(ScaledPeriod(10000, 0.5f, false) == 20000);
    UMP_CHECK(ScaledPeriod(10000, 0.5f, true) == 10000);
    UMP_CHECK(ScaledPeriod(10000, 0.0f, false) == 10000);
    UMP_CHECK(ScaledPeriod(0, 0.5f, false) == 0);
    UMP_CHECK(ScaledCount(2, 0.55f) == 1);
    UMP_CHECK(ScaledCount(2, 1.0f) == 2);
    UMP_CHECK(ScaledCount(0, 1.0f) == 0);
    UMP_CHECK(ScaledCount(1, 0.1f) == 1);
}
