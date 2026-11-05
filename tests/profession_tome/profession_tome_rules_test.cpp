#include "TestMain.h"
#include "ProfessionRules.h"
#include "ProfessionTomeRules.h"
#include <set>
void RunTests()
{
    using namespace ProfessionTome;
    // Each skill once, so the gossip action (the skill id) maps back to a single page.
    std::set<std::uint32_t> skills;
    std::size_t primaries = 0;
    for (auto const& profession : Professions)
    {
        UMP_CHECK(skills.insert(profession.Skill).second);
        UMP_CHECK(Find(profession.Skill) == &profession);
        if (profession.Primary)
            ++primaries;
    }
    UMP_CHECK(primaries == ProfessionRules::Primary.size());
    // The tome covers every primary the all-professions rule unlocks.
    for (auto const& primary : ProfessionRules::Primary)
    {
        Profession const* profession = Find(primary.Skill);
        UMP_CHECK(profession != nullptr);
        UMP_CHECK(profession && profession->Primary);
    }
    // Secondaries: Archaeology, Cooking, First Aid, Fishing.
    for (std::uint32_t skill : { 794u, 185u, 129u, 356u })
        UMP_CHECK(Find(skill) && !Find(skill)->Primary);
    UMP_CHECK(Find(0) == nullptr);
    UMP_CHECK(Find(762) == nullptr); // riding is not a profession
}
