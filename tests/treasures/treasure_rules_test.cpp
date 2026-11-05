#include "TestMain.h"
#include "TreasureRules.h"

#include <cmath>

using namespace Treasures;

namespace
{
    Measurement Good()
    {
        Measurement m;
        m.GroundFound = true;
        m.GroundOffset = 0.2f;
        m.Slope = 5.0f;
        m.Reachable = true;
        m.PathLength = 30.0f;
        m.StraightDistance = 25.0f;
        m.LineOfSight = true;
        m.SkyState = Sky::Covered;
        m.NearestTreasure = 200.0f;
        return m;
    }

    bool Has(std::vector<std::string> const& list, std::string const& text)
    {
        for (std::string const& item : list)
            if (item == text)
                return true;
        return false;
    }

    void SlopeOfFlatAndRamp()
    {
        UMP_CHECK(SlopeDegrees(10, 10, 10, 10, 1) == 0.0f);
        UMP_CHECK(std::fabs(SlopeDegrees(11, 9, 10, 10, 1) - 45.0f) < 0.01f);
        UMP_CHECK(SlopeDegrees(5, 1, 3, 3, 0) == 0.0f);
    }

    void SkyStates()
    {
        UMP_CHECK(SkyFromRays(5, 5, false) == Sky::Covered);
        UMP_CHECK(SkyFromRays(0, 5, true) == Sky::Covered);
        UMP_CHECK(SkyFromRays(2, 5, false) == Sky::Partial);
        UMP_CHECK(SkyFromRays(0, 5, false) == Sky::Open);
    }

    void GoodSpotPasses()
    {
        Verdict v = Judge(Good(), Limits());
        UMP_CHECK(v.Passed() && v.Warnings.empty());
    }

    void EachFailure()
    {
        Limits limits;
        Measurement m = Good();
        m.GroundFound = false;
        UMP_CHECK(Has(Judge(m, limits).Failures, "no ground"));
        m = Good(); m.GroundOffset = 4.0f;
        UMP_CHECK(Has(Judge(m, limits).Failures, "floating or buried"));
        m = Good(); m.Submerged = true;
        UMP_CHECK(Has(Judge(m, limits).Failures, "under liquid"));
        m = Good(); m.Slope = 50.0f;
        UMP_CHECK(Has(Judge(m, limits).Failures, "too steep"));
        m = Good(); m.Reachable = false;
        UMP_CHECK(Has(Judge(m, limits).Failures, "unreachable on foot"));
        m = Good(); m.NearestTreasure = 10.0f;
        UMP_CHECK(Has(Judge(m, limits).Failures, "too close to another treasure"));
        m = Good(); m.NearestTreasure = -1.0f;
        UMP_CHECK(Judge(m, limits).Passed());
    }

    void OpenSkyWarnsUnlessCoverRequired()
    {
        Measurement m = Good();
        m.SkyState = Sky::Open;
        Limits limits;
        Verdict v = Judge(m, limits);
        UMP_CHECK(v.Passed() && Has(v.Warnings, "open sky"));
        limits.RequireCover = true;
        UMP_CHECK(Has(Judge(m, limits).Failures, "open sky"));
    }

    void CoverScoresHigher()
    {
        Measurement covered = Good();
        Measurement open = Good();
        open.SkyState = Sky::Open;
        UMP_CHECK(Judge(covered, Limits()).Score > Judge(open, Limits()).Score);
        Measurement detour = Good();
        detour.PathLength = 200.0f;
        UMP_CHECK(Has(Judge(detour, Limits()).Warnings, "long detour"));
    }

    void NudgeRingsNearestFirst()
    {
        auto offsets = NudgeOffsets(3.0f, 1.5f);
        UMP_CHECK(offsets.size() == 16);
        UMP_CHECK(std::fabs(offsets[0].first - 1.5f) < 0.001f && std::fabs(offsets[0].second) < 0.001f);
        float last = std::hypot(offsets.back().first, offsets.back().second);
        UMP_CHECK(std::fabs(last - 3.0f) < 0.001f);
        UMP_CHECK(NudgeOffsets(3.0f, 0.0f).empty());
        UMP_CHECK(Join({ "a", "b" }) == "a, b");
    }
}

void RunTests()
{
    SlopeOfFlatAndRamp();
    SkyStates();
    GoodSpotPasses();
    EachFailure();
    OpenSkyWarnsUnlessCoverRequired();
    CoverScoresHigher();
    NudgeRingsNearestFirst();
}
