#include "TestMain.h"
#include "ScenarioRules.h"

using namespace Scenarios;

namespace
{
    void Split()
    {
        auto args = SplitArgs("  wander  42 x ");
        UMP_CHECK(args.size() == 3 && args[0] == "wander" && args[1] == "42" && args[2] == "x");
        UMP_CHECK(SplitArgs("").empty());
    }

    void ExpandFills()
    {
        std::map<std::string, std::string> values = { { "target", "Botty" }, { "map", "0" }, { "arg1", "299" } };
        std::string out, missing;
        UMP_CHECK(Expand(".botscenario {target} attack {map} {arg1}", values, out, missing));
        UMP_CHECK(out == ".botscenario Botty attack 0 299");
        UMP_CHECK(Expand("{{literal}", values, out, missing) && out == "{literal}");
    }

    void ExpandRefusesGaps()
    {
        std::map<std::string, std::string> values = { { "target", "" } };
        std::string out, missing;
        UMP_CHECK(!Expand(".bot why {target}", values, out, missing) && missing == "target");
        UMP_CHECK(!Expand(".x {arg2}", values, out, missing) && missing == "arg2");
        UMP_CHECK(!Expand(".x {open", values, out, missing));
    }

    void NoNestedScenarios()
    {
        UMP_CHECK(IsAllowedStep(".clock jump 1d"));
        UMP_CHECK(IsAllowedStep("botscenario Botty stop"));
        UMP_CHECK(!IsAllowedStep(".scenario load other"));
        UMP_CHECK(!IsAllowedStep("  scenario load other"));
        UMP_CHECK(!IsAllowedStep(""));
    }
}

void RunTests()
{
    Split();
    ExpandFills();
    ExpandRefusesGaps();
    NoNestedScenarios();
}
