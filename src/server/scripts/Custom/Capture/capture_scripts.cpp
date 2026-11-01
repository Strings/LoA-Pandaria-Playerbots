/*
 * /idea and /bug in-game capture commands (T-040).
 *
 * Writes structured JSON + Markdown ticket snapshots into Capture.TicketsDir
 * (default: <repo>/tickets/) and provides .capture list / .capture show for
 * inspecting tickets in game.
 */

#include "Custom/Capture/CaptureRules.h"
#include "Custom/Heartseed/HeartseedMgr.h"

#include "AccountMgr.h"
#include "Chat.h"
#include "Config.h"
#include "DBCStores.h"
#include "GitRevision.h"
#include "Group.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "World.h"
#include "WorldSession.h"

#ifdef ENABLE_PLAYERBOTS
#include "BotInspect.h"
#endif

#include <algorithm>
#include <atomic>
#include <chrono>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <shared_mutex>
#include <sstream>

namespace
{
    namespace fs = std::filesystem;

    struct CaptureConfig
    {
        fs::path TicketsDir;
        float NearbyBotRadius = 100.0f;
    };

    std::mutex g_configMutex;
    CaptureConfig g_config;
    std::atomic<uint32_t> g_ticketSeq{0};

    fs::path ResolveTicketsDir(std::string const& configuredPath)
    {
        char const* srcDirRaw = GitRevision::GetSourceDirectory();
        fs::path repoRoot = (srcDirRaw && *srcDirRaw) ? fs::path(srcDirRaw) : fs::current_path();

        if (configuredPath.empty())
            return repoRoot / "tickets";

        fs::path p(configuredPath);
        if (p.is_relative())
            return repoRoot / p;
        return p;
    }

    void LoadCaptureConfig()
    {
        std::string dirSetting = sConfigMgr->GetStringDefault("Capture.TicketsDir", "");
        float radiusSetting = sConfigMgr->GetFloatDefault("Capture.NearbyBotRadius", 100.0f);
        if (radiusSetting < 0.0f)
            radiusSetting = 0.0f;

        std::lock_guard<std::mutex> lock(g_configMutex);
        g_config.TicketsDir = ResolveTicketsDir(dirSetting);
        g_config.NearbyBotRadius = radiusSetting;
    }

    CaptureConfig GetCaptureConfig()
    {
        std::lock_guard<std::mutex> lock(g_configMutex);
        if (g_config.TicketsDir.empty())
        {
            g_config.TicketsDir = ResolveTicketsDir(sConfigMgr->GetStringDefault("Capture.TicketsDir", ""));
            g_config.NearbyBotRadius = std::max(0.0f, sConfigMgr->GetFloatDefault("Capture.NearbyBotRadius", 100.0f));
        }
        return g_config;
    }

    std::string ClassToString(uint8 classId)
    {
        switch (classId)
        {
            case CLASS_WARRIOR:      return "Warrior";
            case CLASS_PALADIN:      return "Paladin";
            case CLASS_HUNTER:       return "Hunter";
            case CLASS_ROGUE:        return "Rogue";
            case CLASS_PRIEST:       return "Priest";
            case CLASS_DEATH_KNIGHT: return "Death Knight";
            case CLASS_SHAMAN:       return "Shaman";
            case CLASS_MAGE:         return "Mage";
            case CLASS_WARLOCK:      return "Warlock";
            case CLASS_MONK:         return "Monk";
            case CLASS_DRUID:        return "Druid";
            default:                 return "Unknown";
        }
    }

    std::string FormatIsoUtc(std::time_t t)
    {
        std::tm utcTm{};
#if defined(_WIN32)
        gmtime_s(&utcTm, &t);
#else
        gmtime_r(&t, &utcTm);
#endif
        std::ostringstream out;
        out << std::setfill('0')
            << std::setw(4) << (utcTm.tm_year + 1900) << '-'
            << std::setw(2) << (utcTm.tm_mon + 1) << '-'
            << std::setw(2) << utcTm.tm_mday << 'T'
            << std::setw(2) << utcTm.tm_hour << ':'
            << std::setw(2) << utcTm.tm_min << ':'
            << std::setw(2) << utcTm.tm_sec << 'Z';
        return out.str();
    }

    Player* ResolveCapturePlayer(ChatHandler* handler)
    {
        if (handler && handler->GetSession() && handler->GetSession()->GetPlayer())
            return handler->GetSession()->GetPlayer();

        if (handler)
        {
            if (Player* sel = handler->getSelectedPlayer())
                return sel;
        }

        // Fallback for SOAP/console commands: prefer an online non-bot player, else any online player.
        Player* fallbackBot = nullptr;
        std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
        for (auto const& pair : ObjectAccessor::GetPlayers())
        {
            Player* p = pair.second;
            if (!p || !p->IsInWorld())
                continue;
#ifdef ENABLE_PLAYERBOTS
            if (BotInspect::IsBot(p))
            {
                if (!fallbackBot)
                    fallbackBot = p;
                continue;
            }
#endif
            return p;
        }
        return fallbackBot;
    }

    Capture::TicketSnapshot BuildSnapshot(Player* player, std::string const& kind, std::string const& rawArgs, float nearbyBotRadius)
    {
        Capture::TicketSnapshot snap;
        auto now = std::chrono::system_clock::now();
        std::time_t nowSec = std::chrono::system_clock::to_time_t(now);
        uint32_t seq = (++g_ticketSeq) % 1000;

        snap.Id = Capture::FormatTicketId(static_cast<int64_t>(nowSec), seq);
        snap.Kind = kind;

        auto parsed = Capture::ParseCommandTextAndScreenshot(rawArgs);
        snap.Text = parsed.first;
        snap.ScreenshotPath = parsed.second;
        snap.RealmTime = FormatIsoUtc(nowSec);

        char const* hash = GitRevision::GetHash();
        char const* fullVer = GitRevision::GetFullVersion();
        snap.ServerCommitHash = hash ? hash : "";
        snap.ServerBuild = fullVer ? fullVer : "";

        snap.Heartseed.Enabled = sHeartseedMgr->IsEnabled();

        if (!player)
            return snap;

        if (WorldSession* sess = player->GetSession())
        {
            snap.AccountId = sess->GetAccountId();
            AccountMgr::GetName(snap.AccountId, snap.AccountName);
        }

        snap.CharacterGuid = player->GetGUID().GetRawValue();
        snap.CharacterName = player->GetName();

        snap.MapId = player->GetMapId();
        if (MapEntry const* mapEntry = sMapStore.LookupEntry(snap.MapId))
        {
            if (mapEntry->name)
                snap.MapName = mapEntry->name;
        }

        uint32 zoneId = 0;
        uint32 areaId = 0;
        player->GetZoneAndAreaId(zoneId, areaId);
        snap.ZoneId = zoneId;
        snap.AreaId = areaId;

        if (AreaTableEntry const* zoneEntry = sAreaTableStore.LookupEntry(zoneId))
        {
            if (zoneEntry->area_name)
                snap.ZoneName = zoneEntry->area_name;
        }
        if (AreaTableEntry const* areaEntry = sAreaTableStore.LookupEntry(areaId))
        {
            if (areaEntry->area_name)
                snap.AreaName = areaEntry->area_name;
        }

        snap.X = player->GetPositionX();
        snap.Y = player->GetPositionY();
        snap.Z = player->GetPositionZ();
        snap.Orientation = player->GetOrientation();

        Unit* target = player->GetSelectedUnit();
        if (!target && !player->GetTarget().IsEmpty())
            target = ObjectAccessor::GetUnit(*player, player->GetTarget());

        if (target)
        {
            snap.Target.HasTarget = true;
            snap.Target.Guid = target->GetGUID().GetRawValue();
            snap.Target.Entry = target->GetEntry();
            snap.Target.Name = target->GetName();
            snap.Target.Health = target->GetHealth();
            snap.Target.MaxHealth = target->GetMaxHealth();
        }

        if (Group const* group = player->GetGroup())
        {
            for (Group::MemberSlot const& slot : group->GetMemberSlots())
            {
                Capture::TicketGroupMember gm;
                gm.Name = slot.name;
                gm.Guid = slot.guid.GetRawValue();
                gm.IsLeader = group->IsLeader(slot.guid);

                if (Player* member = ObjectAccessor::FindPlayer(slot.guid))
                {
                    gm.IsOnline = true;
                    gm.ClassName = ClassToString(member->GetClass());
                    gm.Level = member->GetLevel();
#ifdef ENABLE_PLAYERBOTS
                    gm.IsBot = BotInspect::IsBot(member);
#endif
                }
                snap.GroupMembers.push_back(std::move(gm));
            }
        }

#ifdef ENABLE_PLAYERBOTS
        {
            std::shared_lock<std::shared_mutex> lock(*HashMapHolder<Player>::GetLock());
            for (auto const& pair : ObjectAccessor::GetPlayers())
            {
                Player* other = pair.second;
                if (!other || other == player || !other->IsInWorld())
                    continue;
                if (other->GetMapId() != player->GetMapId() || other->GetInstanceId() != player->GetInstanceId())
                    continue;

                if (!BotInspect::IsBot(other))
                    continue;

                float dist = player->GetExactDist(other);
                if (dist > nearbyBotRadius)
                    continue;

                Capture::TicketBot bot;
                bot.Name = other->GetName();
                bot.ClassName = ClassToString(other->GetClass());
                bot.Level = other->GetLevel();
                bot.Distance = dist;
                bot.Strategies = BotInspect::Strategies(other);
                bot.LastAction = BotInspect::LastAction(other);
                // The traced choice with its score when `.bot why` is tracing this bot, else the last action.
                bot.LastDecision = BotInspect::LastDecisionSummary(other);

                snap.NearbyBots.push_back(std::move(bot));
            }
        }

        std::sort(snap.NearbyBots.begin(), snap.NearbyBots.end(),
            [](Capture::TicketBot const& a, Capture::TicketBot const& b)
            {
                if (a.Distance != b.Distance)
                    return a.Distance < b.Distance;
                return a.Name < b.Name;
            });
#endif

        if (snap.Heartseed.Enabled)
        {
            snap.Heartseed.HasSeed = sHeartseedMgr->HasHeartseed(player);
            Heartseed::State hsState{};
            if (sHeartseedMgr->GetState(player, hsState))
            {
                snap.Heartseed.Rank = hsState.Rank;
                snap.Heartseed.Growth = hsState.Growth;
            }
            snap.Heartseed.EffectiveRank = sHeartseedMgr->GetEffectiveRank(player);
            snap.Heartseed.RankCap = sHeartseedMgr->GetRankCap(player);
        }

        return snap;
    }

    // Minimal string extractor for reading top-level JSON fields back in .capture list
    std::string ExtractJsonStringField(std::string const& jsonContent, std::string const& key)
    {
        std::string needle = "\"" + key + "\": \"";
        std::size_t pos = jsonContent.find(needle);
        if (pos == std::string::npos)
            return "";
        pos += needle.size();
        std::string out;
        for (std::size_t i = pos; i < jsonContent.size(); ++i)
        {
            char c = jsonContent[i];
            if (c == '\\' && i + 1 < jsonContent.size())
            {
                char next = jsonContent[++i];
                switch (next)
                {
                    case '"': out.push_back('"'); break;
                    case '\\': out.push_back('\\'); break;
                    case 'n': out.push_back(' '); break;
                    case 'r': break;
                    case 't': out.push_back(' '); break;
                    default: out.push_back(next); break;
                }
            }
            else if (c == '"')
            {
                break;
            }
            else
            {
                out.push_back(c);
            }
        }
        return out;
    }

    bool WriteTicketAndNotify(ChatHandler* handler, std::string const& kind, char const* args)
    {
        if (!args || !*args)
        {
            handler->PSendSysMessage("Usage: .%s <text>", kind.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        CaptureConfig cfg = GetCaptureConfig();
        Player* player = ResolveCapturePlayer(handler);
        Capture::TicketSnapshot snap = BuildSnapshot(player, kind, args, cfg.NearbyBotRadius);

        if (snap.Text.empty())
        {
            handler->PSendSysMessage("Usage: .%s <text>", kind.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::error_code ec;
        fs::create_directories(cfg.TicketsDir, ec);
        if (ec)
        {
            handler->PSendSysMessage("Capture error: failed to create tickets directory '%s': %s",
                cfg.TicketsDir.string().c_str(), ec.message().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string baseName = Capture::FormatTicketFileName(snap.Id, snap.Kind, snap.Text);
        fs::path jsonPath = cfg.TicketsDir / (baseName + ".json");
        fs::path mdPath = cfg.TicketsDir / (baseName + ".md");

        {
            std::ofstream jsonOut(jsonPath, std::ios::binary | std::ios::trunc);
            if (!jsonOut)
            {
                handler->PSendSysMessage("Capture error: could not write '%s'", jsonPath.string().c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }
            jsonOut << Capture::FormatTicketJson(snap);
        }

        {
            std::ofstream mdOut(mdPath, std::ios::binary | std::ios::trunc);
            if (!mdOut)
            {
                handler->PSendSysMessage("Capture error: could not write '%s'", mdPath.string().c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }
            mdOut << Capture::FormatTicketMarkdown(snap);
        }

        handler->PSendSysMessage("Captured %s [%s] -> %s.json (+ .md)",
            snap.Kind.c_str(), snap.Id.c_str(), baseName.c_str());
        return true;
    }

    std::vector<Capture::TicketListEntry> ScanTickets(fs::path const& ticketsDir)
    {
        std::vector<Capture::TicketListEntry> entries;
        std::error_code ec;
        if (!fs::exists(ticketsDir, ec))
            return entries;

        for (fs::directory_entry const& de : fs::directory_iterator(ticketsDir, ec))
        {
            if (!de.is_regular_file())
                continue;
            fs::path p = de.path();
            if (p.extension() != ".json")
                continue;

            std::ifstream in(p, std::ios::binary);
            if (!in)
                continue;
            std::ostringstream buf;
            buf << in.rdbuf();
            std::string content = buf.str();

            Capture::TicketListEntry item;
            item.FileName = p.stem().string();
            item.Id = ExtractJsonStringField(content, "id");
            item.Kind = ExtractJsonStringField(content, "kind");
            item.RealmTime = ExtractJsonStringField(content, "realm_time");
            item.Text = ExtractJsonStringField(content, "text");

            std::size_t charBlock = content.find("\"character\"");
            if (charBlock != std::string::npos)
                item.CharacterName = ExtractJsonStringField(content.substr(charBlock, 256), "name");

            if (!item.Id.empty())
                entries.push_back(std::move(item));
        }

        std::sort(entries.begin(), entries.end(),
            [](Capture::TicketListEntry const& a, Capture::TicketListEntry const& b)
            {
                return a.Id > b.Id;
            });

        return entries;
    }
}

class capture_worldscript : public WorldScript
{
public:
    capture_worldscript() : WorldScript("capture_worldscript") { }

    void OnConfigLoad(bool /*reload*/) override
    {
        LoadCaptureConfig();
    }
};

class capture_commandscript : public CommandScript
{
public:
    capture_commandscript() : CommandScript("capture_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> captureSubTable =
        {
            { "list", &HandleCaptureListCommand, rbac::RBAC_PERM_COMMAND_TICKET, Trinity::ChatCommands::Console::Yes },
            { "show", &HandleCaptureShowCommand, rbac::RBAC_PERM_COMMAND_TICKET, Trinity::ChatCommands::Console::Yes },
        };

        static std::vector<ChatCommand> commandTable =
        {
            { "idea",    &HandleIdeaCommand, rbac::RBAC_PERM_COMMAND_TICKET, Trinity::ChatCommands::Console::Yes },
            { "bug",     &HandleBugCommand,  rbac::RBAC_PERM_COMMAND_TICKET, Trinity::ChatCommands::Console::Yes },
            { "capture", captureSubTable,    rbac::RBAC_PERM_COMMAND_TICKET, Trinity::ChatCommands::Console::Yes },
        };

        return commandTable;
    }

    static bool HandleIdeaCommand(ChatHandler* handler, char const* args)
    {
        return WriteTicketAndNotify(handler, "idea", args);
    }

    static bool HandleBugCommand(ChatHandler* handler, char const* args)
    {
        return WriteTicketAndNotify(handler, "bug", args);
    }

    static bool HandleCaptureListCommand(ChatHandler* handler, char const* args)
    {
        std::size_t limit = 10;
        if (args && *args)
        {
            long parsed = std::strtol(args, nullptr, 10);
            if (parsed > 0)
                limit = static_cast<std::size_t>(parsed);
        }

        CaptureConfig cfg = GetCaptureConfig();
        std::vector<Capture::TicketListEntry> entries = ScanTickets(cfg.TicketsDir);

        if (entries.empty())
        {
            handler->PSendSysMessage("No captured tickets in %s", cfg.TicketsDir.string().c_str());
            return true;
        }

        std::size_t count = std::min(limit, entries.size());
        handler->PSendSysMessage("Showing %zu of %zu ticket(s) in %s:",
            count, entries.size(), cfg.TicketsDir.string().c_str());

        for (std::size_t i = 0; i < count; ++i)
        {
            auto const& e = entries[i];
            handler->PSendSysMessage("  [%s] (%s) %s @ %s: %s",
                e.Id.c_str(),
                e.Kind.c_str(),
                e.CharacterName.empty() ? "<none>" : e.CharacterName.c_str(),
                e.RealmTime.c_str(),
                e.Text.c_str());
        }
        return true;
    }

    static bool HandleCaptureShowCommand(ChatHandler* handler, char const* args)
    {
        if (!args || !*args)
        {
            handler->SendSysMessage("Usage: .capture show <id>");
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string query = args;
        while (!query.empty() && std::isspace(static_cast<unsigned char>(query.front())))
            query.erase(query.begin());
        while (!query.empty() && std::isspace(static_cast<unsigned char>(query.back())))
            query.pop_back();

        CaptureConfig cfg = GetCaptureConfig();
        std::error_code ec;
        if (!fs::exists(cfg.TicketsDir, ec))
        {
            handler->PSendSysMessage("No ticket matching '%s' (directory does not exist).", query.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        fs::path matchedMd;
        for (fs::directory_entry const& de : fs::directory_iterator(cfg.TicketsDir, ec))
        {
            if (!de.is_regular_file())
                continue;
            fs::path p = de.path();
            if (p.extension() != ".md")
                continue;
            std::string stem = p.stem().string();
            if (stem == "README")
                continue;
            if (stem.find(query) == 0 || stem.find(query) != std::string::npos)
            {
                matchedMd = p;
                break;
            }
        }

        if (matchedMd.empty())
        {
            handler->PSendSysMessage("No ticket matching '%s'.", query.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::ifstream in(matchedMd);
        if (!in)
        {
            handler->PSendSysMessage("Failed to read '%s'.", matchedMd.string().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("=== Ticket %s ===", matchedMd.stem().string().c_str());
        std::string line;
        while (std::getline(in, line))
        {
            if (!line.empty() && line.back() == '\r')
                line.pop_back();
            handler->PSendSysMessage("%s", line.c_str());
        }
        return true;
    }
};

void AddSC_custom_capture()
{
    new capture_worldscript();
    new capture_commandscript();
}
