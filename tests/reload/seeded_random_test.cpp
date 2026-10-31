#include "TestMain.h"
#include "DeterministicRandom.h"
#include "ReloadInput.h"
#include <algorithm>
#include <limits>
#include <thread>
#include <vector>

void RunTests()
{
    std::uint32_t value = 99;
    Trinity::DeterministicRandom random;
    UMP_CHECK(!random.TryNext(value));
    UMP_CHECK(value == 99);
    random.Seed(0);
    UMP_CHECK(random.TryNext(value));
    UMP_CHECK(value == 2357136044u); // Published MT19937 zero-seed raw stream.
    auto first = value;
    UMP_CHECK(random.Inspect().Draws == 1);
    random.Seed(0);
    UMP_CHECK(random.TryNext(value) && value == first);
    random.Seed(1);
    UMP_CHECK(random.TryNext(value) && value == 1791095845u && value != first);
    random.Seed(std::numeric_limits<std::uint32_t>::max());
    UMP_CHECK(random.Inspect().Seed == std::numeric_limits<std::uint32_t>::max());
    random.Disable();
    UMP_CHECK(!random.TryNext(value) && !random.Inspect().Enabled);

    std::uint32_t parsed = 99;
    UMP_CHECK(CustomReload::ParseSeed("0", parsed) && parsed == 0);
    UMP_CHECK(CustomReload::ParseSeed("4294967295", parsed) && parsed == std::numeric_limits<std::uint32_t>::max());
    for (auto invalid : { "", "-1", "+1", "1tail", "4294967296", " 1" })
        UMP_CHECK(!CustomReload::ParseSeed(invalid, parsed));
    UMP_CHECK(parsed == std::numeric_limits<std::uint32_t>::max());

    // Scheduling can change recipients, but concurrent draws must consume exactly one shared stream.
    random.Seed(123);
    std::vector<std::uint32_t> left(128), right(128), serial(256);
    std::thread worker([&] { for (auto& draw : left) random.TryNext(draw); });
    for (auto& draw : right)
        random.TryNext(draw);
    worker.join();
    UMP_CHECK(random.Inspect().Draws == serial.size());
    left.insert(left.end(), right.begin(), right.end());
    random.Seed(123);
    for (auto& draw : serial)
        UMP_CHECK(random.TryNext(draw));
    std::sort(left.begin(), left.end());
    std::sort(serial.begin(), serial.end());
    UMP_CHECK(left == serial);
}
