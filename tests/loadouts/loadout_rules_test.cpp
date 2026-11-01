#include "TestMain.h"
#include "LoadoutRules.h"

using namespace Loadouts;

namespace
{
    void Names()
    {
        UMP_CHECK(ValidName("PvP Frost"));
        UMP_CHECK(ValidName("2v2"));
        UMP_CHECK(!ValidName(""));
        UMP_CHECK(!ValidName("123"));                     // reads as an id
        UMP_CHECK(!ValidName(" lead"));
        UMP_CHECK(!ValidName("trail "));
        UMP_CHECK(!ValidName("a|b"));
        UMP_CHECK(!ValidName("a\tb"));
        UMP_CHECK(!ValidName(std::string(MaxNameLength + 1, 'x')));
        UMP_CHECK(ValidName(std::string(MaxNameLength, 'x')));
    }

    void Groups()
    {
        UMP_CHECK(TargetGroup(0) == 1);
        UMP_CHECK(TargetGroup(1) == 0);
    }

    void Defaults()
    {
        // Druid: four specs, two already have their loadout.
        std::vector<uint16_t> missing = MissingDefaults({ 102, 103, 104, 105 }, { 103, 105 });
        UMP_CHECK(missing.size() == 2 && missing[0] == 102 && missing[1] == 104);
        UMP_CHECK(MissingDefaults({ 62, 0, 64 }, { }).size() == 2);
        UMP_CHECK(MissingDefaults({ 62, 63, 64 }, { 62, 63, 64 }).empty());
    }

    void Ids()
    {
        UMP_CHECK(NextId({ }) == 1);
        UMP_CHECK(NextId({ 1, 2, 7 }) == 8);
        UMP_CHECK(NextId({ 3 }) == 4);
    }

    void ResolveByIdOrName()
    {
        std::vector<Entry> entries = { { 1, "Frost" }, { 2, "Fire" }, { 5, "PvP" }, { 6, "pvp" } };
        UMP_CHECK(Resolve("2", entries) == 2);
        UMP_CHECK(Resolve("3", entries) == 0);
        UMP_CHECK(Resolve("frost", entries) == 1);
        UMP_CHECK(Resolve("FIRE", entries) == 2);
        UMP_CHECK(Resolve("PvP", entries) == 0);          // two match: ambiguous
        UMP_CHECK(Resolve("", entries) == 0);
        UMP_CHECK(Resolve("99999999999", entries) == 0);  // too long for an id, and no such name
    }
}

void RunTests()
{
    Names();
    Groups();
    Defaults();
    Ids();
    ResolveByIdOrName();
}
