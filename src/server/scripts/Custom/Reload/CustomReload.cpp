#include "Chat.h"
#include "Config.h"
#include "Log.h"
#include "Random.h"
#include "ReloadInput.h"
#include "ReloadRegistry.h"
#include "ScriptMgr.h"
#include <sstream>
#include <string>

namespace
{
    bool Fail(ChatHandler* handler, char const* message)
    {
        handler->SendSysMessage(message);
        handler->SetSentErrorMessage(true);
        return false;
    }

    bool ReloadTable(ChatHandler* handler, char const* args)
    {
        std::istringstream input(args ? args : "");
        std::string name, extra;
        if (!(input >> name) || (input >> extra) || !CustomReload::Registry::ValidName(name))
            return Fail(handler, "Usage: reload <registered_table> or reload custom <registered_table>");
        auto result = CustomReload::Tables().Reload(name);
        if (!result.Success)
        {
            TC_LOG_ERROR("server", "Custom reload %s failed: %s", name.c_str(), result.Message.c_str());
            return Fail(handler, result.Message.c_str());
        }
        handler->PSendSysMessage("Reloaded %s (%llu rows): %s", name.c_str(), static_cast<unsigned long long>(result.Rows), result.Message.c_str());
        TC_LOG_INFO("server", "Reloaded custom table %s (%llu rows)", name.c_str(), static_cast<unsigned long long>(result.Rows));
        return true;
    }

    bool ListTables(ChatHandler* handler, char const* args)
    {
        if (args && std::string(args).find_first_not_of(' ') != std::string::npos)
            return Fail(handler, "Usage: reload custom list");
        auto tables = CustomReload::Tables().List();
        if (tables.empty())
            handler->SendSysMessage("No custom table loaders registered.");
        for (auto const& table : tables)
            handler->PSendSysMessage("%s: %s", table.first.c_str(), table.second.c_str());
        return true;
    }

    bool RandomControl(ChatHandler* handler, char const* args)
    {
        std::istringstream input(args ? args : "");
        std::string operation, value, extra;
        input >> operation;
        if (operation == "seed")
        {
            uint32 seed;
            if (!(input >> value) || (input >> extra) || !CustomReload::ParseSeed(value, seed))
                return Fail(handler, "Usage: rng seed <uint32> (0 is a valid seed)");
            SetRandomSeed(seed);
            TC_LOG_INFO("server", "Core test RNG enabled with seed %u", seed);
        }
        else if (operation == "off" || operation == "status" || operation.empty())
        {
            if (input >> extra)
                return Fail(handler, "Usage: rng seed <uint32> | rng off | rng status");
            if (operation == "off")
            {
                ClearRandomSeed();
                TC_LOG_INFO("server", "Core test RNG disabled");
            }
        }
        else
            return Fail(handler, "Usage: rng seed <uint32> | rng off | rng status");
        auto state = GetRandomSeedState();
        handler->PSendSysMessage("Core test RNG: %s; seed=%u; draws=%llu", state.Enabled ? "enabled" : "disabled", state.Seed, static_cast<unsigned long long>(state.Draws));
        return true;
    }

    class reload_framework_commandscript : public CommandScript
    {
    public:
        reload_framework_commandscript() : CommandScript("reload_framework_commandscript") { }

        std::vector<ChatCommand> GetCommands() const override
        {
            using Trinity::ChatCommands::Console;
            std::vector<ChatCommand> custom =
            {
                { "list", &ListTables, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Console::Yes },
                { "", &ReloadTable, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Console::Yes }
            };
            std::vector<ChatCommand> reload =
            {
                { "custom", custom, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Console::Yes },
                { "", &ReloadTable, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Console::Yes }
            };
            return
            {
                { "reload", reload, rbac::RBAC_PERM_COMMAND_RELOAD, Console::Yes },
                { "rng", &RandomControl, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Console::Yes }
            };
        }
    };

    class reload_framework_worldscript : public WorldScript
    {
    public:
        reload_framework_worldscript() : WorldScript("reload_framework_worldscript")
        {
            // Seed before other WorldScripts generate fixtures/bots in OnConfigLoad.
            OnConfigLoad(false);
        }

        void OnConfigLoad(bool /*reload*/) override
        {
            if (!sConfigMgr->GetBoolDefault("Random.TestSeed.Enabled", false))
            {
                ClearRandomSeed();
                return;
            }
            uint32 seed;
            if (!CustomReload::ParseSeed(sConfigMgr->GetStringDefault("Random.TestSeed.Value", "1"), seed))
            {
                TC_LOG_ERROR("server", "Random.TestSeed.Value must be a uint32; retaining previous RNG state");
                return;
            }
            auto state = GetRandomSeedState();
            if (!state.Enabled || state.Seed != seed)
                SetRandomSeed(seed);
        }
    };
}

void AddSC_custom_reload()
{
    new reload_framework_commandscript();
    new reload_framework_worldscript();
}
