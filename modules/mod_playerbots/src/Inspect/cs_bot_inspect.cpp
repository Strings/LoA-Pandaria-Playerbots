/*
 * `.bot` inspection commands (T-039): why, state, trace, plus the sections other bot systems register through
 * BotInspect (traits, memory, skill). See README.md.
 */

#include "BotInspect.h"
#include "Chat.h"
#include "DecisionTrace.h"
#include "Engine.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "PlayerbotAI.h"
#include "Playerbots.h"
#include "RBAC.h"
#include "ScriptMgr.h"

#include <cstdarg>
#include <cstdio>
#include <map>
#include <mutex>

// GM tools block (docs/ID_RANGES.md); linked to role 197 by sql/updates/master/auth/2026_10_31_claude_18_gmtools_t039.sql
static rbac::RBACPermissions const RBAC_PERM_COMMAND_BOT_INSPECT = static_cast<rbac::RBACPermissions>(91004);

namespace
{
    struct Registered
    {
        std::string Description;
        BotInspect::Section Callback;
    };

    std::mutex RegistryLock;

    std::map<std::string, Registered>& Registry()
    {
        static std::map<std::string, Registered> registry;
        return registry;
    }

    // How many queued candidates and evaluated steps `.bot why` prints; the rest are counted.
    size_t const WHY_MAX_LINES = 10;

    char const* const STATE_NAMES[] = { "combat", "non-combat", "dead" };

    std::string Format(char const* format, ...)
    {
        char buffer[512];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        return buffer;
    }

    // Bot from an optional name argument, else the GM's selection.
    Player* ResolveBot(ChatHandler* handler, std::string const& name, PlayerbotAI*& botAI)
    {
        Player* bot = name.empty() ? handler->getSelectedPlayer() : ObjectAccessor::FindPlayerByName(name);
        botAI = bot ? GET_PLAYERBOT_AI(bot) : nullptr;
        if (!bot || !botAI)
        {
            handler->SendSysMessage(name.empty() ? "Select a playerbot or give its name." : "No online playerbot by that name.");
            handler->SetSentErrorMessage(true);
            return nullptr;
        }
        return bot;
    }

    void SetTrace(PlayerbotAI* botAI, bool enabled)
    {
        for (uint8 state = 0; state < BOT_STATE_MAX; ++state)
            if (Engine* engine = botAI->GetEngine(BotState(state)))
                engine->SetDecisionTrace(enabled);
    }

    void PrintDecision(ChatHandler* handler, char const* label, BotDecision const& decision)
    {
        long long ago = (long long)(time(nullptr) - decision.Time);
        if (decision.Chosen.empty())
            handler->PSendSysMessage("%s (%s, %llds ago): nothing ran.", label, decision.EngineState.c_str(), ago);
        else
            handler->PSendSysMessage("%s (%s, %llds ago): ran |cff00ff00%s|r at %.3f.", label, decision.EngineState.c_str(), ago,
                decision.Chosen.c_str(), decision.ChosenRelevance);

        if (!decision.Triggers.empty())
        {
            std::string triggers;
            for (std::string const& trigger : decision.Triggers)
                triggers += (triggers.empty() ? "" : ", ") + trigger;
            handler->PSendSysMessage("  Triggers: %s", triggers.c_str());
        }

        handler->PSendSysMessage("  Candidates (%u, by score):", uint32(decision.Queue.size()));
        for (size_t i = 0; i < decision.Queue.size() && i < WHY_MAX_LINES; ++i)
        {
            DecisionCandidate const& candidate = decision.Queue[i];
            handler->PSendSysMessage("    %.3f  %s%s%s", candidate.Relevance, candidate.Name.c_str(),
                candidate.Source.empty() ? "" : "  <- ", candidate.Source.c_str());
        }
        if (decision.Queue.size() > WHY_MAX_LINES)
            handler->PSendSysMessage("    ... %u more", uint32(decision.Queue.size() - WHY_MAX_LINES));

        handler->PSendSysMessage("  Evaluated (%u, in order):", uint32(decision.Steps.size()));
        for (size_t i = 0; i < decision.Steps.size() && i < WHY_MAX_LINES; ++i)
        {
            DecisionStep const& step = decision.Steps[i];
            handler->PSendSysMessage("    %-10s %.3f  %s", step.Result, step.Relevance, step.Name.c_str());
        }
        if (decision.Steps.size() > WHY_MAX_LINES)
            handler->PSendSysMessage("    ... %u more", uint32(decision.Steps.size() - WHY_MAX_LINES));
    }

    // Engine::GetLastAction is a rolling debug trail ("|PUSH:x - 1.0 (default)|A:x - OK|..."); the newest
    // "A:<name> - OK" entry is the last action that ran.
    std::string LastRanAction(Engine* engine)
    {
        if (!engine)
            return "";
        std::string const trail = engine->GetLastAction();
        size_t end = trail.rfind(" - OK");
        while (end != std::string::npos)
        {
            size_t start = trail.rfind("|A:", end);
            if (start != std::string::npos && trail.find('|', start + 1) > end)
                return trail.substr(start + 3, end - start - 3);
            end = end ? trail.rfind(" - OK", end - 1) : std::string::npos;
        }
        return "";
    }

    // "why", "why <name>", "why off", "why off <name>": splits an optional leading keyword from the bot name.
    std::string TakeWord(char const* args, std::string& rest)
    {
        std::string text = args ? args : "";
        size_t start = text.find_first_not_of(' ');
        if (start == std::string::npos)
        {
            rest.clear();
            return "";
        }
        size_t end = text.find(' ', start);
        std::string word = text.substr(start, end == std::string::npos ? std::string::npos : end - start);
        size_t next = end == std::string::npos ? std::string::npos : text.find_first_not_of(' ', end);
        rest = next == std::string::npos ? "" : text.substr(next);
        return word;
    }
}

namespace BotInspect
{
    bool Register(std::string const& name, std::string const& description, Section section)
    {
        if (name.empty() || !section)
            return false;
        std::lock_guard<std::mutex> guard(RegistryLock);
        return Registry().emplace(name, Registered{ description, std::move(section) }).second;
    }

    std::vector<Planned> const& PlannedSections()
    {
        // The `.bot` sections DESIGN.md asks for ("GM tools (decided)", Inspect bots), before their systems exist.
        static std::vector<Planned> const planned =
        {
            { "traits", "the personality traits system (DESIGN.md, Human-like bots)" },
            { "memory", "the bot memory system (DESIGN.md, Human-like bots)" },
            { "skill",  "the bot skill model (DESIGN.md, Human-like bots)" },
        };
        return planned;
    }

    bool Explain(std::string const& name, Player* bot, std::vector<std::string>& lines)
    {
        Section callback;
        {
            std::lock_guard<std::mutex> guard(RegistryLock);
            auto itr = Registry().find(name);
            if (itr != Registry().end())
                callback = itr->second.Callback;
        }

        if (!callback)
        {
            for (Planned const& planned : PlannedSections())
                if (name == planned.Name)
                {
                    lines.push_back(Format("No %s yet: this section is filled by %s.", planned.Name, planned.Arrives));
                    return false;
                }
            lines.push_back(Format("Unknown section '%s'.", name.c_str()));
            return false;
        }

        callback(bot, lines);
        return true;
    }

    bool IsBot(Player* player)
    {
        return player && GET_PLAYERBOT_AI(player);
    }

    std::vector<std::string> Strategies(Player* bot)
    {
        PlayerbotAI* botAI = bot ? GET_PLAYERBOT_AI(bot) : nullptr;
        return botAI ? botAI->GetStrategies(botAI->GetState()) : std::vector<std::string>();
    }

    std::string LastAction(Player* bot)
    {
        PlayerbotAI* botAI = bot ? GET_PLAYERBOT_AI(bot) : nullptr;
        return botAI ? LastRanAction(botAI->GetEngine(botAI->GetState())) : "";
    }

    std::string LastDecisionSummary(Player* bot)
    {
        PlayerbotAI* botAI = bot ? GET_PLAYERBOT_AI(bot) : nullptr;
        if (!botAI)
            return "";

        Engine* engine = botAI->GetEngine(botAI->GetState());
        if (!engine)
            return "";

        if (DecisionTrace const* trace = engine->GetDecisionTrace())
            if (trace->HasLast())
                return Format("%s (%.3f, traced)", trace->Last().Chosen.c_str(), trace->Last().ChosenRelevance);

        return LastRanAction(engine);
    }
}

class playerbots_inspect_commandscript : public CommandScript
{
public:
    playerbots_inspect_commandscript() : CommandScript("playerbots_inspect_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> botCommandTable =
        {
            { "why",      &HandleWhyCommand,      RBAC_PERM_COMMAND_BOT_INSPECT, Trinity::ChatCommands::Console::No },
            { "state",    &HandleStateCommand,    RBAC_PERM_COMMAND_BOT_INSPECT, Trinity::ChatCommands::Console::No },
            { "sections", &HandleSectionsCommand, RBAC_PERM_COMMAND_BOT_INSPECT, Trinity::ChatCommands::Console::Yes },
            { "traits",   &HandleTraitsCommand,   RBAC_PERM_COMMAND_BOT_INSPECT, Trinity::ChatCommands::Console::No },
            { "memory",   &HandleMemoryCommand,   RBAC_PERM_COMMAND_BOT_INSPECT, Trinity::ChatCommands::Console::No },
            { "skill",    &HandleSkillCommand,    RBAC_PERM_COMMAND_BOT_INSPECT, Trinity::ChatCommands::Console::No },
            { "explain",  &HandleExplainCommand,  RBAC_PERM_COMMAND_BOT_INSPECT, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "bot", botCommandTable, RBAC_PERM_COMMAND_BOT_INSPECT, Trinity::ChatCommands::Console::Yes },
        };
        return commandTable;
    }

    static bool HandleWhyCommand(ChatHandler* handler, char const* args)
    {
        std::string rest;
        std::string word = TakeWord(args, rest);
        bool off = word == "off";
        std::string name = off ? rest : (args ? std::string(args) : "");
        name = TakeWord(name.c_str(), rest);

        PlayerbotAI* botAI = nullptr;
        Player* bot = ResolveBot(handler, name, botAI);
        if (!bot)
            return false;

        if (off)
        {
            SetTrace(botAI, false);
            handler->PSendSysMessage("Stopped tracing %s.", bot->GetName().c_str());
            return true;
        }

        Engine* engine = botAI->GetEngine(botAI->GetState());
        DecisionTrace const* trace = engine ? engine->GetDecisionTrace() : nullptr;
        if (!trace)
        {
            SetTrace(botAI, true);
            handler->PSendSysMessage("Tracing %s now (last action: %s). Ask again in a moment; `.bot why off` stops it.",
                bot->GetName().c_str(), LastRanAction(engine).empty() ? "none recorded" : LastRanAction(engine).c_str());
            return true;
        }

        BotState state = botAI->GetState();
        handler->PSendSysMessage("%s, %s engine, %llu traced ticks.", bot->GetName().c_str(),
            state < BOT_STATE_MAX ? STATE_NAMES[state] : "unknown", (unsigned long long)trace->Ticks());
        if (trace->HasLast())
            PrintDecision(handler, "Last decision", trace->Last());
        else
            handler->SendSysMessage("No action has run in this engine since tracing started.");

        if (trace->LastIdle().Time > (trace->HasLast() ? trace->Last().Time : 0))
            PrintDecision(handler, "Latest idle tick", trace->LastIdle());
        return true;
    }

    static bool HandleStateCommand(ChatHandler* handler, char const* args)
    {
        std::string rest;
        PlayerbotAI* botAI = nullptr;
        Player* bot = ResolveBot(handler, TakeWord(args, rest), botAI);
        if (!bot)
            return false;

        BotState state = botAI->GetState();
        handler->PSendSysMessage("%s: level %u, %s, state %s.", bot->GetName().c_str(), uint32(bot->GetLevel()),
            bot->IsAlive() ? "alive" : "dead", state < BOT_STATE_MAX ? STATE_NAMES[state] : "unknown");
        for (uint8 i = 0; i < BOT_STATE_MAX; ++i)
        {
            Engine* engine = botAI->GetEngine(BotState(i));
            if (!engine)
                continue;
            handler->PSendSysMessage("  %s%s: last action %s; %s", STATE_NAMES[i], i == state ? " (current)" : "",
                LastRanAction(engine).empty() ? "none recorded" : LastRanAction(engine).c_str(),
                engine->ListStrategies().c_str());
        }
        return true;
    }

    static bool HandleSectionsCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->SendSysMessage("Built in: why, state. Sections:");
        {
            std::lock_guard<std::mutex> guard(RegistryLock);
            for (auto const& [name, registered] : Registry())
                handler->PSendSysMessage("  %s - %s", name.c_str(), registered.Description.c_str());
        }
        for (BotInspect::Planned const& planned : BotInspect::PlannedSections())
        {
            std::lock_guard<std::mutex> guard(RegistryLock);
            if (!Registry().count(planned.Name))
                handler->PSendSysMessage("  %s - not built yet (%s)", planned.Name, planned.Arrives);
        }
        return true;
    }

    static bool ExplainSection(ChatHandler* handler, std::string const& section, std::string const& name)
    {
        PlayerbotAI* botAI = nullptr;
        Player* bot = ResolveBot(handler, name, botAI);
        if (!bot)
            return false;

        std::vector<std::string> lines;
        BotInspect::Explain(section, bot, lines);
        handler->PSendSysMessage("%s, %s:", bot->GetName().c_str(), section.c_str());
        for (std::string const& line : lines)
            handler->PSendSysMessage("  %s", line.c_str());
        return true;
    }

    static bool HandleTraitsCommand(ChatHandler* handler, char const* args)
    {
        std::string rest;
        return ExplainSection(handler, "traits", TakeWord(args, rest));
    }

    static bool HandleMemoryCommand(ChatHandler* handler, char const* args)
    {
        std::string rest;
        return ExplainSection(handler, "memory", TakeWord(args, rest));
    }

    static bool HandleSkillCommand(ChatHandler* handler, char const* args)
    {
        std::string rest;
        return ExplainSection(handler, "skill", TakeWord(args, rest));
    }

    // `.bot explain <section> [name]` for sections without their own subcommand.
    static bool HandleExplainCommand(ChatHandler* handler, char const* args)
    {
        std::string rest;
        std::string section = TakeWord(args, rest);
        if (section.empty())
        {
            handler->SendSysMessage("Usage: .bot explain <section> [bot name]; `.bot sections` lists them.");
            handler->SetSentErrorMessage(true);
            return false;
        }
        std::string ignored;
        return ExplainSection(handler, section, TakeWord(rest.c_str(), ignored));
    }
};

void AddSC_playerbots_inspect_commandscript()
{
    new playerbots_inspect_commandscript();
}
