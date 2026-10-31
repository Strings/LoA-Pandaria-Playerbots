/*
 * Scenario loader (T-039): `.scenario load <name> [args]` sets up a whole test situation in one command. A scenario
 * is data: ordered GM command lines with placeholders, run as the calling GM, so it can drive .botscenario, .tele,
 * .checkpoint, .clock and any later system's commands without new code. See README.md.
 */

#include "Chat.h"
#include "DatabaseEnv.h"
#include "Log.h"
#include "Player.h"
#include "RBAC.h"
#include "Custom/Reload/ReloadRegistry.h"
#include "ScenarioRules.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

#include <memory>
#include <mutex>

// GM tools block (docs/ID_RANGES.md); linked to role 197 by sql/updates/master/auth/2026_10_31_claude_18_gmtools_t039.sql
static rbac::RBACPermissions const RBAC_PERM_COMMAND_SCENARIO = static_cast<rbac::RBACPermissions>(91005);

namespace
{
    struct Step
    {
        uint8 Order = 0;
        std::string Command;
        std::string Comment;
    };

    struct Scenario
    {
        std::string Name;
        std::string Description;
        std::string Usage;
        std::vector<Step> Steps;
    };

    using Snapshot = std::map<std::string, Scenario>;

    std::mutex SnapshotLock;
    std::shared_ptr<Snapshot const> Current = std::make_shared<Snapshot const>();

    std::shared_ptr<Snapshot const> Get()
    {
        std::lock_guard<std::mutex> guard(SnapshotLock);
        return Current;
    }

    // Validates into a private snapshot and publishes only a complete one; any bad row keeps the old cache.
    CustomReload::Result Load()
    {
        auto next = std::make_shared<Snapshot>();
        std::size_t rows = 0;

        QueryResult scenarios = WorldDatabase.Query("SELECT `name`, `description`, `usage` FROM `gmtools_scenario`");
        if (scenarios)
        {
            do
            {
                Field* fields = scenarios->Fetch();
                Scenario scenario;
                scenario.Name = fields[0].GetString();
                scenario.Description = fields[1].GetString();
                scenario.Usage = fields[2].GetString();
                (*next)[scenario.Name] = scenario;
                ++rows;
            } while (scenarios->NextRow());
        }

        QueryResult steps = WorldDatabase.Query("SELECT `scenario`, `step`, `command`, `comment` FROM `gmtools_scenario_step` ORDER BY `scenario`, `step`");
        if (steps)
        {
            do
            {
                Field* fields = steps->Fetch();
                std::string name = fields[0].GetString();
                auto itr = next->find(name);
                if (itr == next->end())
                {
                    TC_LOG_ERROR("server", "gmtools_scenario_step: step %u belongs to unknown scenario '%s'", uint32(fields[1].GetUInt8()), name.c_str());
                    return { false, 0, "A step names an unknown scenario; previous cache retained (see server log)" };
                }

                Step step{ fields[1].GetUInt8(), fields[2].GetString(), fields[3].GetString() };
                if (!Scenarios::IsAllowedStep(step.Command))
                {
                    TC_LOG_ERROR("server", "gmtools_scenario_step: '%s' step %u starts another scenario", name.c_str(), uint32(step.Order));
                    return { false, 0, "A step starts another scenario; previous cache retained (see server log)" };
                }
                itr->second.Steps.push_back(step);
                ++rows;
            } while (steps->NextRow());
        }

        {
            std::lock_guard<std::mutex> guard(SnapshotLock);
            Current = std::move(next);
        }
        return { true, rows, "Scenarios published" };
    }

    std::map<std::string, std::string> Placeholders(Player* player, std::vector<std::string> const& args)
    {
        std::map<std::string, std::string> values;
        values["player"] = player->GetName();
        if (Unit* target = player->GetSelectedUnit())
            if (target != player)
                values["target"] = target->GetName();
        values["map"] = std::to_string(player->GetMapId());
        values["zone"] = std::to_string(player->GetZoneId());
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.2f", player->GetPositionX());
        values["x"] = buffer;
        snprintf(buffer, sizeof(buffer), "%.2f", player->GetPositionY());
        values["y"] = buffer;
        snprintf(buffer, sizeof(buffer), "%.2f", player->GetPositionZ());
        values["z"] = buffer;
        snprintf(buffer, sizeof(buffer), "%.3f", player->GetOrientation());
        values["o"] = buffer;
        for (size_t i = 0; i < args.size(); ++i)
            values["arg" + std::to_string(i + 1)] = args[i];
        return values;
    }
}

class gmtools_scenario_commandscript : public CommandScript
{
public:
    gmtools_scenario_commandscript() : CommandScript("gmtools_scenario_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> scenarioCommandTable =
        {
            { "list",    &HandleListCommand,    RBAC_PERM_COMMAND_SCENARIO, Trinity::ChatCommands::Console::Yes },
            { "explain", &HandleExplainCommand, RBAC_PERM_COMMAND_SCENARIO, Trinity::ChatCommands::Console::Yes },
            { "load",    &HandleLoadCommand,    RBAC_PERM_COMMAND_SCENARIO, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "scenario", scenarioCommandTable, RBAC_PERM_COMMAND_SCENARIO, Trinity::ChatCommands::Console::Yes },
        };
        return commandTable;
    }

    static bool HandleListCommand(ChatHandler* handler, char const* /*args*/)
    {
        auto snapshot = Get();
        handler->PSendSysMessage("%u scenarios:", uint32(snapshot->size()));
        for (auto const& [name, scenario] : *snapshot)
            handler->PSendSysMessage("  %s - %s", name.c_str(), scenario.Description.c_str());
        return true;
    }

    static Scenario const* Find(ChatHandler* handler, std::shared_ptr<Snapshot const> const& snapshot, std::string const& name)
    {
        auto itr = snapshot->find(name);
        if (itr == snapshot->end())
        {
            handler->PSendSysMessage("No scenario '%s'; `.scenario list` shows them.", name.c_str());
            handler->SetSentErrorMessage(true);
            return nullptr;
        }
        return &itr->second;
    }

    static bool HandleExplainCommand(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> words = Scenarios::SplitArgs(args ? args : "");
        if (words.empty())
        {
            handler->SendSysMessage("Usage: .scenario explain <name>");
            handler->SetSentErrorMessage(true);
            return false;
        }

        auto snapshot = Get();
        Scenario const* scenario = Find(handler, snapshot, words[0]);
        if (!scenario)
            return false;

        handler->PSendSysMessage("%s: %s", scenario->Name.c_str(), scenario->Description.c_str());
        if (!scenario->Usage.empty())
            handler->PSendSysMessage("  Usage: .scenario load %s %s", scenario->Name.c_str(), scenario->Usage.c_str());
        for (Step const& step : scenario->Steps)
            handler->PSendSysMessage("  %u. %s%s%s", uint32(step.Order), step.Command.c_str(),
                step.Comment.empty() ? "" : "  -- ", step.Comment.c_str());
        return true;
    }

    static bool HandleLoadCommand(ChatHandler* handler, char const* args)
    {
        std::vector<std::string> words = Scenarios::SplitArgs(args ? args : "");
        if (words.empty())
        {
            handler->SendSysMessage("Usage: .scenario load <name> [args]");
            handler->SetSentErrorMessage(true);
            return false;
        }

        auto snapshot = Get();
        Scenario const* scenario = Find(handler, snapshot, words[0]);
        if (!scenario)
            return false;

        Player* player = handler->GetSession()->GetPlayer();
        std::map<std::string, std::string> values = Placeholders(player, std::vector<std::string>(words.begin() + 1, words.end()));

        // Expand every step first, so a missing argument stops the scenario before anything has run.
        std::vector<std::string> commands;
        for (Step const& step : scenario->Steps)
        {
            std::string command, missing;
            if (!Scenarios::Expand(step.Command, values, command, missing))
            {
                handler->PSendSysMessage("Scenario %s needs {%s} for step %u; nothing ran.%s%s", scenario->Name.c_str(),
                    missing.c_str(), uint32(step.Order), scenario->Usage.empty() ? "" : " Usage: ", scenario->Usage.c_str());
                handler->SetSentErrorMessage(true);
                return false;
            }
            if (command[0] != '.')
                command = "." + command;
            commands.push_back(command);
        }

        handler->PSendSysMessage("Scenario %s: running %u steps.", scenario->Name.c_str(), uint32(commands.size()));
        ChatHandler stepHandler(handler->GetSession());
        for (size_t i = 0; i < commands.size(); ++i)
        {
            handler->PSendSysMessage("  %u. %s", uint32(scenario->Steps[i].Order), commands[i].c_str());
            stepHandler.ParseCommands(commands[i].c_str());
        }
        return true;
    }
};

class gmtools_scenario_worldscript : public WorldScript
{
public:
    gmtools_scenario_worldscript() : WorldScript("gmtools_scenario_worldscript") { }

    void OnStartup() override
    {
        CustomReload::Result result = CustomReload::Tables().Reload("gmtools_scenario");
        if (result.Success)
            TC_LOG_INFO("server.loading", ">> Loaded %u GM scenario rows", uint32(result.Rows));
        else
            TC_LOG_ERROR("server", "GM scenarios failed to load: %s", result.Message.c_str());
    }
};

void AddSC_gmtools_scenarios()
{
    if (!CustomReload::Tables().RegisterGroup({ "gmtools_scenario", "gmtools_scenario_step" }, "GM test scenarios (T-039)", [] { return Load(); }))
        TC_LOG_ERROR("server", "Scenario reload loader registration failed");
    new gmtools_scenario_worldscript();
    new gmtools_scenario_commandscript();
}
