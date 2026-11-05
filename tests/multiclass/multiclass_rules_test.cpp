#include "TestMain.h"
#include "MulticlassRules.h"

using namespace Multiclass;

namespace
{
    void LevelCost()
    {
        UMP_CHECK(XpPerLevel(100000, 25) == 25000);
        UMP_CHECK(XpPerLevel(0, 25) == 1);                           // never free
        UMP_CHECK(XpPerLevel(UINT32_MAX, 200) == UINT32_MAX);
    }

    void Leveling()
    {
        Progress p = AddXp({ 1, 0 }, 2500, 1000, 30);
        UMP_CHECK(p.Level == 3 && p.Xp == 500);
        p = AddXp({ 29, 900 }, 5000, 1000, 30);                      // stops at the cap
        UMP_CHECK(p.Level == 30 && p.Xp == 0);
        p = AddXp({ 30, 0 }, 5000, 1000, 30);
        UMP_CHECK(p.Level == 30 && p.Xp == 0);
        p = AddXp({ 1, 0 }, 999, 1000, 30);
        UMP_CHECK(p.Level == 1 && p.Xp == 999);
        p = AddXp({ 1, 0 }, UINT64_MAX / 2, 1, 255);                 // no wrap
        UMP_CHECK(p.Level == 255);
    }

    void RuleCooldowns()
    {
        UMP_CHECK(RuleReady(0, 5000, 10000));
        UMP_CHECK(!RuleReady(1000, 5000, 10000) && RuleReady(1000, 11000, 10000));
        UMP_CHECK(RuleReady(1000, 1001, 0));
        UMP_CHECK(RuleReady(UINT32_MAX - 10, 9990, 10000));          // across the clock wrap
    }
}

void RunTests()
{
    LevelCost();
    Leveling();
    RuleCooldowns();
}
