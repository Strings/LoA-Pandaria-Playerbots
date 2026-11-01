/*
 * Pure ticket formatting and parsing rules for /idea and /bug capture (T-040).
 * No server or database dependencies so unit tests can exercise these directly.
 */

#ifndef CAPTURE_RULES_H
#define CAPTURE_RULES_H

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

namespace Capture
{
    struct TicketTarget
    {
        bool HasTarget = false;
        uint64_t Guid = 0;
        uint32_t Entry = 0;
        std::string Name;
        uint32_t Health = 0;
        uint32_t MaxHealth = 0;
    };

    struct TicketGroupMember
    {
        std::string Name;
        uint64_t Guid = 0;
        std::string ClassName;
        uint8_t Level = 0;
        bool IsLeader = false;
        bool IsBot = false;
        bool IsOnline = false;
    };

    struct TicketBot
    {
        std::string Name;
        std::string ClassName;
        uint8_t Level = 0;
        float Distance = 0.0f;
        std::vector<std::string> Strategies;
        std::string LastAction;
        // Reserved for T-039 (.bot why decision trace); filled from LastAction for now.
        std::string LastDecision;
    };

    struct TicketHeartseed
    {
        bool Enabled = false;
        bool HasSeed = false;
        uint8_t Rank = 0;
        uint8_t EffectiveRank = 0;
        uint8_t RankCap = 0;
        uint32_t Growth = 0;
    };

    struct TicketSnapshot
    {
        std::string Id;
        std::string Kind;               // "idea" or "bug"
        std::string Text;
        std::string RealmTime;          // ISO-8601 UTC timestamp
        uint32_t AccountId = 0;
        std::string AccountName;
        uint64_t CharacterGuid = 0;
        std::string CharacterName;
        uint32_t MapId = 0;
        std::string MapName;
        uint32_t ZoneId = 0;
        std::string ZoneName;
        uint32_t AreaId = 0;
        std::string AreaName;
        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;
        float Orientation = 0.0f;
        TicketTarget Target;
        std::vector<TicketGroupMember> GroupMembers;
        std::vector<TicketBot> NearbyBots;
        TicketHeartseed Heartseed;
        std::string ServerCommitHash;
        std::string ServerBuild;
        std::string ScreenshotPath;
    };

    struct TicketListEntry
    {
        std::string Id;
        std::string Kind;
        std::string RealmTime;
        std::string CharacterName;
        std::string Text;
        std::string FileName;
    };

    std::string EscapeJson(std::string const& input);
    std::string SanitizeSlug(std::string const& text, std::size_t maxLen = 32);
    std::string FormatTicketId(int64_t unixTimeUtc, uint32_t seq);
    std::string FormatTicketFileName(std::string const& id, std::string const& kind, std::string const& text);
    bool IsValidTicketId(std::string const& id);

    // Parses "[screenshot=WoWScrnShot_MMDDYY_HHMMSS] <text>" or "--screenshot=..." prefix/suffix from command args.
    // Returns (cleaned text, screenshot path).
    std::pair<std::string, std::string> ParseCommandTextAndScreenshot(std::string const& rawArgs);

    // Cleans Engine::GetLastAction() ring buffer ("|T:...|A:action - OK") down to the most recent action entry.
    std::string ExtractLastAction(std::string const& rawEngineAction);

    std::string FormatTicketJson(TicketSnapshot const& snap);
    std::string FormatTicketMarkdown(TicketSnapshot const& snap);
}

#endif
