#include "TestMain.h"
#include "CaptureRules.h"

using namespace Capture;

namespace
{
    void TicketIdAndValidation()
    {
        // 2026-10-31T22:30:15Z -> 1793485815
        std::string id = FormatTicketId(1793485815LL, 7);
        UMP_CHECK(id == "20261031-223015-007");
        UMP_CHECK(IsValidTicketId(id));
        UMP_CHECK(IsValidTicketId("20261031-223015-007_idea_test"));
        UMP_CHECK(!IsValidTicketId(""));
        UMP_CHECK(!IsValidTicketId("../20261031-223015-007"));
        UMP_CHECK(!IsValidTicketId("20261031-223015-007/../../etc"));
        UMP_CHECK(!IsValidTicketId("not-a-ticket-id"));
    }

    void FileNameAndSlugSanitization()
    {
        UMP_CHECK(SanitizeSlug("  Quest giver floats above the bridge!! ") == "quest-giver-floats-above-the-bri");
        UMP_CHECK(SanitizeSlug("!!!") == "ticket");
        UMP_CHECK(SanitizeSlug("Short Note", 16) == "short-note");

        std::string fname = FormatTicketFileName("20261031-223015-001", "bug", "Boss resets on evade!");
        UMP_CHECK(fname == "20261031-223015-001_bug_boss-resets-on-evade");

        std::string fallbackKind = FormatTicketFileName("20261031-223015-002", "other", "Add guild tabard rack");
        UMP_CHECK(fallbackKind == "20261031-223015-002_idea_add-guild-tabard-rack");
    }

    void JsonEscaping()
    {
        UMP_CHECK(EscapeJson("plain text") == "plain text");
        UMP_CHECK(EscapeJson("quote \" and slash \\") == "quote \\\" and slash \\\\");
        UMP_CHECK(EscapeJson("line1\nline2\r\tend") == "line1\\nline2\\r\\tend");
        std::string ctrl(1, '\x01');
        UMP_CHECK(EscapeJson(ctrl) == "\\u0001");
    }

    void CommandScreenshotParsing()
    {
        auto p1 = ParseCommandTextAndScreenshot("[screenshot=WoWScrnShot_103126_183015] Quest NPC stuck in rock");
        UMP_CHECK(p1.first == "Quest NPC stuck in rock");
        UMP_CHECK(p1.second == "WoWScrnShot_103126_183015");

        auto p2 = ParseCommandTextAndScreenshot("--screenshot=WoWScrnShot_103126_183016.jpg Herb node underground");
        UMP_CHECK(p2.first == "Herb node underground");
        UMP_CHECK(p2.second == "WoWScrnShot_103126_183016.jpg");

        auto p3 = ParseCommandTextAndScreenshot("Simple idea without screenshot");
        UMP_CHECK(p3.first == "Simple idea without screenshot");
        UMP_CHECK(p3.second.empty());

        // Reject path traversal in screenshot token
        auto p4 = ParseCommandTextAndScreenshot("[screenshot=../secret/file] Idea text");
        UMP_CHECK(p4.second.empty());
    }

    void SnapshotSerialization()
    {
        TicketSnapshot snap;
        snap.Id = "20261031-223015-001";
        snap.Kind = "idea";
        snap.Text = "Bots should wave \"hello\" at campfires";
        snap.RealmTime = "2026-10-31T22:30:15Z";
        snap.AccountId = 1;
        snap.AccountName = "ADMIN";
        snap.CharacterGuid = 42;
        snap.CharacterName = "Tester";
        snap.MapId = 0;
        snap.MapName = "Eastern Kingdoms";
        snap.ZoneId = 12;
        snap.ZoneName = "Elwynn Forest";
        snap.AreaId = 87;
        snap.AreaName = "Goldshire";
        snap.X = -9464.0f;
        snap.Y = 62.0f;
        snap.Z = 56.0f;
        snap.Orientation = 1.5f;
        snap.Target.HasTarget = true;
        snap.Target.Guid = 999;
        snap.Target.Entry = 240;
        snap.Target.Name = "Marshal Dughan";
        snap.Target.Health = 4500;
        snap.Target.MaxHealth = 4500;

        TicketBot bot;
        bot.Name = "Botalpha";
        bot.ClassName = "Warrior";
        bot.Level = 10;
        bot.Distance = 12.5f;
        bot.Strategies = { "dps", "close" };
        bot.LastAction = "melee - OK";
        bot.LastDecision = bot.LastAction;
        snap.NearbyBots.push_back(bot);

        snap.Heartseed.Enabled = true;
        snap.Heartseed.HasSeed = true;
        snap.Heartseed.Rank = 3;
        snap.Heartseed.EffectiveRank = 3;
        snap.Heartseed.RankCap = 10;
        snap.Heartseed.Growth = 250;
        snap.ServerCommitHash = "e68d7112";
        snap.ServerBuild = "UltimateMoP";
        snap.ScreenshotPath = "WoWScrnShot_103126_183015";

        std::string json = FormatTicketJson(snap);
        UMP_CHECK(json.find("\"id\": \"20261031-223015-001\"") != std::string::npos);
        UMP_CHECK(json.find("\"kind\": \"idea\"") != std::string::npos);
        UMP_CHECK(json.find("Bots should wave \\\"hello\\\" at campfires") != std::string::npos);
        UMP_CHECK(json.find("\"last_action\": \"melee - OK\"") != std::string::npos);
        UMP_CHECK(json.find("\"last_decision\": \"melee - OK\"") != std::string::npos);
        UMP_CHECK(json.find("\"rank\": 3") != std::string::npos);
        UMP_CHECK(json.find("\"screenshot_path\": \"WoWScrnShot_103126_183015\"") != std::string::npos);

        std::string md = FormatTicketMarkdown(snap);
        UMP_CHECK(md.find("# [IDEA] 20261031-223015-001") != std::string::npos);
        UMP_CHECK(md.find("last_decision=`melee - OK`") != std::string::npos);
        UMP_CHECK(md.find("```json") != std::string::npos);
    }
}

void RunTests()
{
    TicketIdAndValidation();
    FileNameAndSlugSanitization();
    JsonEscaping();
    CommandScreenshotParsing();
    SnapshotSerialization();
}
