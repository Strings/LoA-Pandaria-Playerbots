#include "TestMain.h"
#include "BindingRules.h"

void RunTests()
{
    using namespace BindingRules;
    Item ordinary;
    ordinary.Bonding = 1;
    UMP_CHECK(Suppress(true, ordinary));
    UMP_CHECK(!Suppress(false, ordinary));
    for (std::uint32_t binding : { 0u, 1u, 2u, 3u })
    {
        ordinary.Bonding = binding;
        UMP_CHECK(Suppress(true, ordinary));
    }
    for (std::uint32_t binding : { 4u, 5u, UINT32_MAX })
    {
        ordinary.Bonding = binding;
        UMP_CHECK(!Suppress(true, ordinary));
    }
    Item quest;
    quest.Class = 12;
    UMP_CHECK(Protected(quest));
    Item starter;
    starter.StartQuest = 1;
    UMP_CHECK(Protected(starter));
    Item artifact;
    artifact.Quality = 6;
    UMP_CHECK(Protected(artifact));
    Item configured;
    configured.ConfiguredProtection = true;
    UMP_CHECK(Protected(configured));
    Item copy;
    copy.SystemProtection = true;
    UMP_CHECK(Protected(copy));
    Item unknown;
    unknown.Known = false;
    UMP_CHECK(Protected(unknown));
    UMP_CHECK(TooltipFlags(UINT32_MAX, true) == (UINT32_MAX ^ 0x08000000u));
    UMP_CHECK(TooltipFlags(UINT32_MAX, false) == UINT32_MAX);
    UMP_CHECK(NextVersion(100, 100) == 101);
    UMP_CHECK(NextVersion(100, 99) == 101);
    UMP_CHECK(NextVersion(100, 1000) == 1000);
    UMP_CHECK(NextVersion(UINT32_MAX, 100) == UINT32_MAX);
}
