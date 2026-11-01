/*
 * Pure ticket formatting and parsing rules for /idea and /bug capture (T-040).
 */

#include "CaptureRules.h"

#include <cctype>
#include <cstdio>
#include <ctime>
#include <sstream>

namespace Capture
{
    namespace
    {
        std::string Trim(std::string const& s)
        {
            std::size_t start = 0;
            while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
                ++start;
            std::size_t end = s.size();
            while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
                --end;
            return s.substr(start, end - start);
        }

        std::string FormatFloat(float v)
        {
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%.2f", v);
            return buf;
        }

        void UtcTimeComponents(int64_t unixTimeUtc, int& year, int& month, int& day, int& hour, int& minute, int& second)
        {
            time_t t = static_cast<time_t>(unixTimeUtc < 0 ? 0 : unixTimeUtc);
            struct tm utcTm = {};
#if defined(_WIN32)
            gmtime_s(&utcTm, &t);
#else
            gmtime_r(&t, &utcTm);
#endif
            year = utcTm.tm_year + 1900;
            month = utcTm.tm_mon + 1;
            day = utcTm.tm_mday;
            hour = utcTm.tm_hour;
            minute = utcTm.tm_min;
            second = utcTm.tm_sec;
        }

        bool IsSafeScreenshotName(std::string const& s)
        {
            if (s.empty() || s.size() > 128)
                return false;
            if (s.find("..") != std::string::npos || s.find('/') != std::string::npos || s.find('\\') != std::string::npos)
                return false;
            for (unsigned char c : s)
            {
                if (!std::isalnum(c) && c != '_' && c != '-' && c != '.')
                    return false;
            }
            return true;
        }
    }

    std::string EscapeJson(std::string const& input)
    {
        std::string out;
        out.reserve(input.size() + 8);
        for (unsigned char c : input)
        {
            switch (c)
            {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\b': out += "\\b"; break;
                case '\f': out += "\\f"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (c < 0x20)
                    {
                        char hex[8];
                        std::snprintf(hex, sizeof(hex), "\\u%04x", static_cast<unsigned int>(c));
                        out += hex;
                    }
                    else
                    {
                        out.push_back(static_cast<char>(c));
                    }
                    break;
            }
        }
        return out;
    }

    std::string SanitizeSlug(std::string const& text, std::size_t maxLen)
    {
        std::string slug;
        slug.reserve(maxLen);
        bool prevDash = true;
        for (unsigned char c : text)
        {
            if (std::isalnum(c))
            {
                if (slug.size() >= maxLen)
                    break;
                slug.push_back(static_cast<char>(std::tolower(c)));
                prevDash = false;
            }
            else if (!prevDash && slug.size() < maxLen)
            {
                slug.push_back('-');
                prevDash = true;
            }
        }
        while (!slug.empty() && slug.back() == '-')
            slug.pop_back();
        if (slug.empty())
            slug = "ticket";
        return slug;
    }

    std::string FormatTicketId(int64_t unixTimeUtc, uint32_t seq)
    {
        int year = 1970, month = 1, day = 1, hour = 0, minute = 0, second = 0;
        UtcTimeComponents(unixTimeUtc, year, month, day, hour, minute, second);
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%04d%02d%02d-%02d%02d%02d-%03u",
            year, month, day, hour, minute, second, static_cast<unsigned int>(seq % 1000));
        return buf;
    }

    std::string FormatTicketFileName(std::string const& id, std::string const& kind, std::string const& text)
    {
        std::string safeKind = (kind == "bug") ? "bug" : "idea";
        std::string slug = SanitizeSlug(text, 32);
        return id + "_" + safeKind + "_" + slug;
    }

    bool IsValidTicketId(std::string const& id)
    {
        // Requires at least YYYYMMDD-HHMMSS-NNN (19 chars) and no path separators or '..'
        if (id.size() < 19 || id.size() > 80)
            return false;
        if (id.find("..") != std::string::npos)
            return false;
        for (std::size_t i = 0; i < 19; ++i)
        {
            if (i == 8 || i == 15)
            {
                if (id[i] != '-')
                    return false;
            }
            else if (!std::isdigit(static_cast<unsigned char>(id[i])))
            {
                return false;
            }
        }
        for (std::size_t i = 19; i < id.size(); ++i)
        {
            unsigned char c = static_cast<unsigned char>(id[i]);
            if (!std::isalnum(c) && c != '-' && c != '_')
                return false;
        }
        return true;
    }

    std::pair<std::string, std::string> ParseCommandTextAndScreenshot(std::string const& rawArgs)
    {
        std::string trimmed = Trim(rawArgs);
        if (trimmed.empty())
            return { "", "" };

        // Support prefix "[screenshot=<name>] <text>" or "--screenshot=<name> <text>"
        if (trimmed.compare(0, 12, "[screenshot=") == 0)
        {
            std::size_t closeBracket = trimmed.find(']', 12);
            if (closeBracket != std::string::npos)
            {
                std::string shot = Trim(trimmed.substr(12, closeBracket - 12));
                std::string rest = Trim(trimmed.substr(closeBracket + 1));
                if (!IsSafeScreenshotName(shot))
                    shot.clear();
                return { rest, shot };
            }
        }
        else if (trimmed.compare(0, 13, "--screenshot=") == 0)
        {
            std::size_t spacePos = trimmed.find_first_of(" \t", 13);
            if (spacePos != std::string::npos)
            {
                std::string shot = Trim(trimmed.substr(13, spacePos - 13));
                std::string rest = Trim(trimmed.substr(spacePos + 1));
                if (!IsSafeScreenshotName(shot))
                    shot.clear();
                return { rest, shot };
            }
            std::string shot = Trim(trimmed.substr(13));
            if (!IsSafeScreenshotName(shot))
                shot.clear();
            return { "", shot };
        }

        // Also support suffix "<text> [screenshot=<name>]"
        if (trimmed.back() == ']')
        {
            std::size_t openBracket = trimmed.rfind("[screenshot=");
            if (openBracket != std::string::npos)
            {
                std::string shot = Trim(trimmed.substr(openBracket + 12, trimmed.size() - openBracket - 13));
                std::string rest = Trim(trimmed.substr(0, openBracket));
                if (!IsSafeScreenshotName(shot))
                    shot.clear();
                return { rest, shot };
            }
        }

        return { trimmed, "" };
    }

    std::string FormatTicketJson(TicketSnapshot const& snap)
    {
        std::ostringstream out;
        out << "{\n";
        out << "  \"id\": \"" << EscapeJson(snap.Id) << "\",\n";
        out << "  \"kind\": \"" << EscapeJson(snap.Kind) << "\",\n";
        out << "  \"text\": \"" << EscapeJson(snap.Text) << "\",\n";
        out << "  \"realm_time\": \"" << EscapeJson(snap.RealmTime) << "\",\n";
        out << "  \"account\": {\n";
        out << "    \"id\": " << snap.AccountId << ",\n";
        out << "    \"name\": \"" << EscapeJson(snap.AccountName) << "\"\n";
        out << "  },\n";
        out << "  \"character\": {\n";
        out << "    \"guid\": " << snap.CharacterGuid << ",\n";
        out << "    \"name\": \"" << EscapeJson(snap.CharacterName) << "\"\n";
        out << "  },\n";
        out << "  \"location\": {\n";
        out << "    \"map_id\": " << snap.MapId << ",\n";
        out << "    \"map_name\": \"" << EscapeJson(snap.MapName) << "\",\n";
        out << "    \"zone_id\": " << snap.ZoneId << ",\n";
        out << "    \"zone_name\": \"" << EscapeJson(snap.ZoneName) << "\",\n";
        out << "    \"area_id\": " << snap.AreaId << ",\n";
        out << "    \"area_name\": \"" << EscapeJson(snap.AreaName) << "\",\n";
        out << "    \"x\": " << FormatFloat(snap.X) << ",\n";
        out << "    \"y\": " << FormatFloat(snap.Y) << ",\n";
        out << "    \"z\": " << FormatFloat(snap.Z) << ",\n";
        out << "    \"o\": " << FormatFloat(snap.Orientation) << "\n";
        out << "  },\n";

        if (snap.Target.HasTarget)
        {
            out << "  \"target\": {\n";
            out << "    \"guid\": " << snap.Target.Guid << ",\n";
            out << "    \"entry\": " << snap.Target.Entry << ",\n";
            out << "    \"name\": \"" << EscapeJson(snap.Target.Name) << "\",\n";
            out << "    \"health\": " << snap.Target.Health << ",\n";
            out << "    \"max_health\": " << snap.Target.MaxHealth << "\n";
            out << "  },\n";
        }
        else
        {
            out << "  \"target\": null,\n";
        }

        out << "  \"group_members\": [";
        for (std::size_t i = 0; i < snap.GroupMembers.size(); ++i)
        {
            auto const& m = snap.GroupMembers[i];
            out << (i == 0 ? "\n" : ",\n");
            out << "    {\n";
            out << "      \"name\": \"" << EscapeJson(m.Name) << "\",\n";
            out << "      \"guid\": " << m.Guid << ",\n";
            out << "      \"class\": \"" << EscapeJson(m.ClassName) << "\",\n";
            out << "      \"level\": " << static_cast<unsigned int>(m.Level) << ",\n";
            out << "      \"is_leader\": " << (m.IsLeader ? "true" : "false") << ",\n";
            out << "      \"is_bot\": " << (m.IsBot ? "true" : "false") << ",\n";
            out << "      \"is_online\": " << (m.IsOnline ? "true" : "false") << "\n";
            out << "    }";
        }
        if (!snap.GroupMembers.empty())
            out << "\n  ";
        out << "],\n";

        out << "  \"nearby_bots\": [";
        for (std::size_t i = 0; i < snap.NearbyBots.size(); ++i)
        {
            auto const& b = snap.NearbyBots[i];
            out << (i == 0 ? "\n" : ",\n");
            out << "    {\n";
            out << "      \"name\": \"" << EscapeJson(b.Name) << "\",\n";
            out << "      \"class\": \"" << EscapeJson(b.ClassName) << "\",\n";
            out << "      \"level\": " << static_cast<unsigned int>(b.Level) << ",\n";
            out << "      \"distance\": " << FormatFloat(b.Distance) << ",\n";
            out << "      \"strategies\": [";
            for (std::size_t s = 0; s < b.Strategies.size(); ++s)
            {
                if (s > 0)
                    out << ", ";
                out << "\"" << EscapeJson(b.Strategies[s]) << "\"";
            }
            out << "],\n";
            out << "      \"last_action\": \"" << EscapeJson(b.LastAction) << "\",\n";
            out << "      \"last_decision\": \"" << EscapeJson(b.LastDecision) << "\"\n";
            out << "    }";
        }
        if (!snap.NearbyBots.empty())
            out << "\n  ";
        out << "],\n";

        out << "  \"custom_systems\": {\n";
        out << "    \"heartseed\": {\n";
        out << "      \"enabled\": " << (snap.Heartseed.Enabled ? "true" : "false") << ",\n";
        out << "      \"has_seed\": " << (snap.Heartseed.HasSeed ? "true" : "false") << ",\n";
        out << "      \"rank\": " << static_cast<unsigned int>(snap.Heartseed.Rank) << ",\n";
        out << "      \"effective_rank\": " << static_cast<unsigned int>(snap.Heartseed.EffectiveRank) << ",\n";
        out << "      \"rank_cap\": " << static_cast<unsigned int>(snap.Heartseed.RankCap) << ",\n";
        out << "      \"growth\": " << snap.Heartseed.Growth << "\n";
        out << "    }\n";
        out << "  },\n";

        out << "  \"server\": {\n";
        out << "    \"commit_hash\": \"" << EscapeJson(snap.ServerCommitHash) << "\",\n";
        out << "    \"build\": \"" << EscapeJson(snap.ServerBuild) << "\"\n";
        out << "  },\n";
        out << "  \"screenshot_path\": \"" << EscapeJson(snap.ScreenshotPath) << "\"\n";
        out << "}\n";
        return out.str();
    }

    std::string FormatTicketMarkdown(TicketSnapshot const& snap)
    {
        std::string upperKind = snap.Kind;
        for (char& c : upperKind)
            c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));

        std::ostringstream out;
        out << "# [" << upperKind << "] " << snap.Id << "\n\n";
        out << "**Text:** " << snap.Text << "\n\n";
        out << "- **Time:** " << snap.RealmTime << "\n";
        out << "- **Character:** " << snap.CharacterName << " (guid " << snap.CharacterGuid
            << ", account " << snap.AccountName << " #" << snap.AccountId << ")\n";
        out << "- **Location:** map " << snap.MapId << " (" << snap.MapName << "), zone " << snap.ZoneId
            << " (" << snap.ZoneName << "), area " << snap.AreaId << " (" << snap.AreaName << ") at ("
            << FormatFloat(snap.X) << ", " << FormatFloat(snap.Y) << ", " << FormatFloat(snap.Z)
            << ", o=" << FormatFloat(snap.Orientation) << ")\n";
        if (snap.Target.HasTarget)
        {
            out << "- **Target:** " << snap.Target.Name << " (guid " << snap.Target.Guid
                << ", entry " << snap.Target.Entry << ", hp " << snap.Target.Health << "/" << snap.Target.MaxHealth << ")\n";
        }
        else
        {
            out << "- **Target:** none\n";
        }
        out << "- **Heartseed:** " << (snap.Heartseed.HasSeed ? "yes" : "no")
            << " (rank " << static_cast<unsigned int>(snap.Heartseed.Rank)
            << ", effective " << static_cast<unsigned int>(snap.Heartseed.EffectiveRank)
            << "/" << static_cast<unsigned int>(snap.Heartseed.RankCap)
            << ", growth " << snap.Heartseed.Growth << ")\n";
        out << "- **Nearby bots:** " << snap.NearbyBots.size() << "\n";
        for (auto const& b : snap.NearbyBots)
        {
            out << "  - " << b.Name << " (" << b.ClassName << " L" << static_cast<unsigned int>(b.Level)
                << ", " << FormatFloat(b.Distance) << "y): last_action=`" << b.LastAction
                << "`, last_decision=`" << b.LastDecision << "`\n";
        }
        out << "- **Group members:** " << snap.GroupMembers.size() << "\n";
        out << "- **Screenshot:** " << (snap.ScreenshotPath.empty() ? "none" : snap.ScreenshotPath) << "\n";
        out << "- **Server:** " << snap.ServerCommitHash << " (" << snap.ServerBuild << ")\n\n";
        out << "```json\n" << FormatTicketJson(snap) << "```\n";
        return out.str();
    }
}
