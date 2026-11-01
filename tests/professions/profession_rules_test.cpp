#include "TestMain.h"
#include "ProfessionRules.h"
#include <set>
void RunTests()
{
    using namespace ProfessionRules;
    UMP_CHECK(FreeSlots(11, 0) == 11);
    UMP_CHECK(FreeSlots(11, 2) == 9);
    UMP_CHECK(FreeSlots(11, 11) == 0);
    UMP_CHECK(FreeSlots(2, 11) == 0);
    UMP_CHECK(FreeSlots(0, 0) == 0);
    UMP_CHECK(FreeSlots(UINT32_MAX, 0) == 11);
    UMP_CHECK(FreeSlots(11, UINT32_MAX) == 0);
    std::set<std::uint32_t> skills, spells;
    for (auto const& profession : Primary)
    {
        UMP_CHECK(skills.insert(profession.Skill).second);
        UMP_CHECK(spells.insert(profession.FirstSpell).second);
    }
    UMP_CHECK(skills.size() == 11);
}
