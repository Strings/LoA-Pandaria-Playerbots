#include "TestMain.h"
#include "AreaProbeRules.h"

using namespace AreaProbe;

namespace
{
    bool Near(float a, float b, float tolerance = 0.01f) { return std::fabs(a - b) <= tolerance; }

    template <typename Inside>
    std::vector<Sample> Probe(float step, uint32_t rings, uint32_t rays, Inside inside)
    {
        std::vector<Sample> samples;
        for (Slot const& slot : Layout(step, rings, rays, 2.0f * Pi))
        {
            Point at{ slot.Distance * std::cos(slot.Angle), slot.Distance * std::sin(slot.Angle) };
            samples.push_back({ at, slot.Ray, inside(slot.Distance, slot.Angle, at) });
        }
        return samples;
    }

    void Layouts()
    {
        std::vector<Slot> full = Layout(2.0f, 3, 4, 2.0f * Pi);
        UMP_CHECK(full.size() == 12);
        UMP_CHECK(Near(full[0].Distance, 2.0f) && Near(full[2].Distance, 6.0f) && full[2].Ray == 0);
        UMP_CHECK(Near(full[3].Angle, Pi / 2.0f) && Near(std::fabs(full[6].Angle), Pi) && Near(full[9].Angle, -Pi / 2.0f));

        std::vector<Slot> arc = Layout(1.0f, 1, 3, Pi / 2.0f);
        UMP_CHECK(arc.size() == 3 && Near(arc[0].Angle, -Pi / 4.0f) && Near(arc[1].Angle, 0.0f) && Near(arc[2].Angle, Pi / 4.0f));
        UMP_CHECK(Layout(0.0f, 3, 4, Pi).empty() && Layout(1.0f, 0, 4, Pi).empty());
    }

    void LocalFrame()
    {
        // Facing north (+y): a point north is straight ahead, one west is to the left.
        Point ahead = ToLocal(10.0f, 10.0f, Pi / 2.0f, 10.0f, 15.0f);
        UMP_CHECK(Near(ahead.X, 5.0f) && Near(ahead.Y, 0.0f));
        Point left = ToLocal(10.0f, 10.0f, Pi / 2.0f, 5.0f, 10.0f);
        UMP_CHECK(Near(left.X, 0.0f) && Near(left.Y, 5.0f));
    }

    void FitsCone()
    {
        // 12 yards, 45 degrees either side; spokes every 15 degrees, rings every yard.
        Fit fit = Analyze(Probe(1.0f, 20, 24, [](float d, float a, Point) { return d <= 12.0f && std::fabs(a) <= Pi / 4.0f + 1e-3f; }));
        UMP_CHECK(fit.RaysHit == 7 && fit.Rays == 24 && !fit.FullCircle && !fit.Gaps);
        UMP_CHECK(Near(fit.Center, 0.0f));
        UMP_CHECK(Near(fit.Reach.Low, 12.0f) && Near(fit.Reach.High, 13.0f));
        UMP_CHECK(Near(fit.HalfAngle.Low, Pi / 4.0f) && Near(fit.HalfAngle.High, Pi / 3.0f));
    }

    void FitsRearCone()
    {
        Fit fit = Analyze(Probe(1.0f, 10, 24, [](float d, float a, Point) { return d <= 5.0f && std::fabs(a) >= 3.0f * Pi / 4.0f - 1e-3f; }));
        UMP_CHECK(Near(std::fabs(fit.Center), Pi));
        UMP_CHECK(Near(fit.HalfAngle.Low, Pi / 4.0f) && Near(fit.HalfAngle.High, Pi / 3.0f));
    }

    void FitsCircleAroundCaster()
    {
        Fit fit = Analyze(Probe(1.0f, 12, 16, [](float d, float, Point) { return d <= 8.0f; }));
        UMP_CHECK(fit.FullCircle && Near(fit.HalfAngle.Low, Pi));
        UMP_CHECK(Near(fit.Reach.Low, 8.0f) && Near(fit.Reach.High, 9.0f));
        UMP_CHECK(Near(fit.CircleCenter.X, 0.0f, 0.05f) && Near(fit.CircleCenter.Y, 0.0f, 0.05f));
        UMP_CHECK(Near(fit.Radius.Low, 8.0f, 0.05f) && Near(fit.Radius.High, 9.0f, 0.05f));
    }

    void FitsCircleAwayFromCaster()
    {
        // A ground circle of 5 yards, 10 yards ahead: the free circle finds it within a yard.
        Fit fit = Analyze(Probe(1.0f, 20, 48, [](float, float, Point p) { return Length({ p.X - 10.0f, p.Y }) <= 5.0f; }));
        UMP_CHECK(!fit.FullCircle && Near(fit.Center, 0.0f));
        UMP_CHECK(Near(fit.CircleCenter.X, 10.0f, 1.0f) && Near(fit.CircleCenter.Y, 0.0f, 0.05f));
        UMP_CHECK(fit.Radius.Low <= 5.0f + 1.0f && fit.Radius.High >= 5.0f - 1.0f);
        UMP_CHECK(fit.Gaps);  // nothing close to the caster was hit
    }

    void NoHits()
    {
        Fit fit = Analyze(Probe(1.0f, 4, 8, [](float, float, Point) { return false; }));
        UMP_CHECK(fit.Hits == 0 && fit.Misses == 32 && fit.RaysHit == 0 && fit.Rays == 8);
    }
}

void RunTests()
{
    Layouts();
    LocalFrame();
    FitsCone();
    FitsRearCone();
    FitsCircleAroundCaster();
    FitsCircleAwayFromCaster();
    NoHits();
}
