#include "TestMain.h"
#include "ReloadRegistry.h"
#include <future>
#include <thread>

void RunTests()
{
    CustomReload::Registry registry;
    UMP_CHECK(CustomReload::Registry::ValidName(std::string(64, 'a')));
    UMP_CHECK(!CustomReload::Registry::ValidName(std::string(65, 'a')));
    UMP_CHECK(!CustomReload::Registry::ValidName("Uppercase"));
    UMP_CHECK(!CustomReload::Registry::ValidName(""));
    int published = 10;
    bool valid = false;
    UMP_CHECK(registry.Register("test_traits", "Staged loader", [&]() -> CustomReload::Result
    {
        int staged = 20;
        if (!valid)
            return { false, 0, "Invalid input; old cache retained" };
        published = staged;
        return { true, 1, "Published" };
    }));
    UMP_CHECK(!registry.Register("test_traits", "Duplicate", [] { return CustomReload::Result{ true, 0, "" }; }));
    UMP_CHECK(!registry.Register("test_traits;drop", "Invalid", [] { return CustomReload::Result{ true, 0, "" }; }));
    UMP_CHECK(!registry.Reload("unknown").Success);
    UMP_CHECK(!registry.Reload("test_traits").Success);
    UMP_CHECK(published == 10);
    valid = true;
    UMP_CHECK(registry.Reload("test_traits").Success);
    UMP_CHECK(published == 20);

    UMP_CHECK(registry.Register("test_exception", "Exception", []() -> CustomReload::Result
    {
        throw std::runtime_error("load failed");
    }));
    UMP_CHECK(!registry.Reload("test_exception").Success);
    UMP_CHECK(!registry.Reload("test_exception").Success); // Failure released its reload guard.
    UMP_CHECK(registry.Register("test_recursive", "No recursive reload", [&]() -> CustomReload::Result
    {
        UMP_CHECK(!registry.Reload("test_recursive").Success);
        UMP_CHECK(!registry.List().empty());
        return { true, 0, "" };
    }));
    UMP_CHECK(registry.Reload("test_recursive").Success);

    std::promise<void> entered, release;
    auto ready = entered.get_future();
    auto proceed = release.get_future();
    UMP_CHECK(registry.RegisterGroup({ "test_concurrent", "test_concurrent_alias" }, "Single loader", [&]() -> CustomReload::Result
    {
        entered.set_value();
        proceed.wait();
        return { true, 1, "" };
    }));
    CustomReload::Result first{ false, 0, "" };
    std::thread worker([&] { first = registry.Reload("test_concurrent"); });
    ready.wait();
    bool rejected = !registry.Reload("test_concurrent_alias").Success;
    auto listing = registry.List();
    release.set_value();
    worker.join();
    UMP_CHECK(rejected && first.Success && listing.size() == 5);

    UMP_CHECK(registry.RegisterGroup({ "test_stage", "test_glow" }, "Coherent cache", [&]() -> CustomReload::Result
    {
        UMP_CHECK(!registry.Reload("test_glow").Success);
        return { true, 2, "One snapshot" };
    }));
    UMP_CHECK(registry.Reload("test_stage").Success);
    UMP_CHECK(!registry.RegisterGroup({ "test_stage", "test_unpublished" }, "Conflict", []
    {
        return CustomReload::Result{ true, 0, "" };
    }));
    UMP_CHECK(!registry.Reload("test_unpublished").Success);
    UMP_CHECK(!registry.RegisterGroup({ "test_duplicate", "test_duplicate" }, "Duplicate aliases", []
    {
        return CustomReload::Result{ true, 0, "" };
    }));
}
