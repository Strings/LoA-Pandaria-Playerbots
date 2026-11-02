#include "TestMain.h"
#include "TelemetryCore.h"

#include <cmath>

using namespace Telemetry;

namespace
{
    bool Near(float a, float b) { return std::fabs(a - b) < 0.001f; }

    void JsonRows()
    {
        JsonObject row;
        row.Add("v", SchemaVersion).Add("t", "move").Add("x", 1.25, 2).Add("n", int8_t(-3)).Add("ok", true);
        UMP_CHECK(row.Str() == "{\"v\":1,\"t\":\"move\",\"x\":1.25,\"n\":-3,\"ok\":true}");
        UMP_CHECK(JsonObject().Empty() && JsonObject().Str() == "{}");

        UMP_CHECK(JsonEscape("say \"hi\"\n\\") == "say \\\"hi\\\"\\n\\\\");
        UMP_CHECK(JsonEscape(std::string_view("\x01", 1)) == "\\u0001");
        UMP_CHECK(JsonEscape("caf\xC3\xA9") == "caf\xC3\xA9");           // UTF-8 passes through

        UMP_CHECK(JsonNumber(2.500, 3) == "2.5" && JsonNumber(3.0, 2) == "3" && JsonNumber(-0.001, 2) == "0");
        UMP_CHECK(JsonNumber(std::nan(""), 2) == "null");

        JsonArray list;
        list.Add(uint32_t(7)).Add(0.5, 1).AddRaw("{}");
        UMP_CHECK(list.Str() == "[7,0.5,{}]" && list.Size() == 3);
        UMP_CHECK(JsonObject().AddRaw("a", JsonArray().Str()).AddNull("b").Str() == "{\"a\":[],\"b\":null}");
    }

    void PauseReasons()
    {
        PauseState pause;
        UMP_CHECK(pause.Set(PAUSE_MANUAL, true) && pause.Paused());
        UMP_CHECK(!pause.Set(PAUSE_AFK, true));                       // already paused: no new gap edge
        UMP_CHECK(!pause.Set(PAUSE_MANUAL, false) && pause.Paused()); // AFK still holds
        UMP_CHECK(PauseReasonNames(pause.Reasons()) == "afk");
        UMP_CHECK(pause.Set(PAUSE_AFK, false) && !pause.Paused());

        pause.Set(PAUSE_GM_COMMAND, true);
        pause.Set(PAUSE_IDLE, true);
        UMP_CHECK(pause.OnInput() && !pause.Paused());                // moving clears both
        pause.Set(PAUSE_MANUAL, true);
        UMP_CHECK(!pause.OnInput() && pause.Paused());                // but never a manual pause
        UMP_CHECK(PauseReasonNames(PAUSE_MANUAL | PAUSE_GM_MODE) == "manual,gm_mode");
    }

    void DiscardWindow()
    {
        RowBuffer rows;
        for (uint64_t t = 0; t < 10; ++t)
            rows.Push(t * 1000, "r" + std::to_string(t));
        std::vector<std::string> done = rows.TakeFinal(9000, 5000);   // rows from 0 to 4 s are final
        UMP_CHECK(done.size() == 5 && done.front() == "r0" && done.back() == "r4");
        UMP_CHECK(rows.Discard(9000, 2000) == 3 && rows.Size() == 2); // 7, 8 and 9 s go
        UMP_CHECK(rows.Discard(9000, 0) == 0);
        std::vector<std::string> rest = rows.TakeAll();
        UMP_CHECK(rest.size() == 2 && rest[0] == "r5" && rows.Size() == 0);
        UMP_CHECK(rows.Discard(1000, 5000) == 0);                      // never underflows
    }

    void Activities()
    {
        ActivityInputs in;
        UMP_CHECK(ClassifyActivity(in) == Activity::Questing);
        in.Traveling = true;
        UMP_CHECK(ClassifyActivity(in) == Activity::Travel);
        in.Idle = true;
        UMP_CHECK(ClassifyActivity(in) == Activity::Idle);
        in.AtService = true;
        UMP_CHECK(ClassifyActivity(in) == Activity::Vendor);
        in.InCombat = true;
        UMP_CHECK(ClassifyActivity(in) == Activity::Fight && std::string(ActivityName(Activity::Fight)) == "fight");
    }

    void Episodes()
    {
        EpisodeTracker ep;
        UMP_CHECK(ep.Update(Activity::Questing, 0, 1000, 3000) && ep.Id() == 1);
        UMP_CHECK(!ep.Update(Activity::Travel, 0, 2000, 3000));        // not held long enough
        UMP_CHECK(!ep.Update(Activity::Questing, 0, 2500, 3000));      // a flicker resets the hold
        UMP_CHECK(!ep.Update(Activity::Travel, 0, 3000, 3000));
        UMP_CHECK(ep.Update(Activity::Travel, 0, 6000, 3000) && ep.Id() == 2 && ep.StartMs() == 3000);
        UMP_CHECK(ep.Update(Activity::Fight, 0, 6100, 3000) && ep.Current() == Activity::Fight);   // fights at once
        UMP_CHECK(ep.Update(Activity::Fight, 2, 6200, 3000) && ep.Context() == 2);                 // so do context changes
        UMP_CHECK(!ep.Update(Activity::Fight, 2, 9000, 3000) && ep.Id() == 4);
        ep.Break();
        UMP_CHECK(ep.Update(Activity::Fight, 2, 9500, 3000) && ep.Id() == 5 && ep.StartMs() == 9500);
        UMP_CHECK(!ep.Update(Activity::Fight, 2, 9600, 3000));
    }

    void MovementSampling()
    {
        MoveSample a;
        MoveSample b = a;
        UMP_CHECK(!MoveChanged(a, b) && !MoveSampleDue(a, 0, b, 10000, 200));   // standing still
        b.X = 0.01f;
        UMP_CHECK(!MoveChanged(a, b));                                          // under 5 cm
        b.X = 1.0f;
        UMP_CHECK(!MoveSampleDue(a, 1000, b, 1100, 200) && MoveSampleDue(a, 1000, b, 1200, 200));
        MoveSample turned = a;
        turned.O = 0.5f;
        UMP_CHECK(MoveSampleDue(a, 0, turned, 300, 200));                       // facing alone counts
        MoveSample strafe = a;
        strafe.Flags = 0x4;
        UMP_CHECK(MoveSampleDue(a, 1000, strafe, 1001, 200));                   // flag edges log at once
    }

    void Geometry()
    {
        float pi = 3.14159265f;
        UMP_CHECK(Near(NormalizeAngle(3.0f * pi), pi) || Near(NormalizeAngle(3.0f * pi), -pi));
        UMP_CHECK(Near(NormalizeAngle(-0.5f), -0.5f) && Near(NormalizeAngle(2.0f * pi + 0.25f), 0.25f));
        UMP_CHECK(Near(RelativeAngle(0, 0, 0, 10, 0), 0.0f));
        UMP_CHECK(Near(RelativeAngle(0, 0, 0, 0, 10), pi / 2));                // left is positive
        UMP_CHECK(Near(RelativeAngle(0, 0, pi / 2, 10, 0), -pi / 2));
        UMP_CHECK(Near(RelativeAngle(5, 5, 1, 5, 5), 0.0f));

        UMP_CHECK(SummarizePack({ 0, 0, 0 }, {}).Count == 0);
        PackSummary pack = SummarizePack({ 0, 0, 0 }, { { 10, 2, 0.1f }, { 10, -2, -0.1f } });
        UMP_CHECK(pack.Count == 2 && Near(pack.Distance, 10.0f) && Near(pack.Angle, 0.0f) && Near(pack.Heading, 0.0f));
        // Headings either side of north (0 and 2 pi) average to 0, not pi.
        PackSummary wrap = SummarizePack({ 0, 0, 0 }, { { 0, 5, 0.2f }, { 0, 5, 2.0f * pi - 0.2f } });
        UMP_CHECK(Near(wrap.Heading, 0.0f) && Near(wrap.Angle, pi / 2));

        UMP_CHECK(Near(CastProgress(2000, 500), 0.75f) && Near(CastProgress(2000, 3000), 0.0f));
        UMP_CHECK(Near(CastProgress(0, 0), 1.0f) && Near(CastProgress(1500, -10), 1.0f));
    }

    void Fights()
    {
        FightTracker fight;
        uint64_t since = 0;
        UMP_CHECK(!fight.SinceEnd(500, since));
        fight.AddTaken(99);                                             // outside a fight: ignored
        fight.Start(1000);
        fight.Start(1500);                                              // already running: keeps 1000
        fight.AddTaken(300);
        fight.AddDone(700);
        fight.AddKill();
        UMP_CHECK(!fight.SinceEnd(2000, since));
        FightSummary s = fight.End(4000);
        UMP_CHECK(s.DurationMs == 3000 && s.DamageTaken == 300 && s.DamageDone == 700 && s.Kills == 1);
        UMP_CHECK(fight.SinceEnd(6500, since) && since == 2500);
        UMP_CHECK(fight.End(7000).DurationMs == 0);                     // ending twice reports nothing
    }
}

void RunTests()
{
    JsonRows();
    PauseReasons();
    DiscardWindow();
    Activities();
    Episodes();
    MovementSampling();
    Geometry();
    Fights();
}
