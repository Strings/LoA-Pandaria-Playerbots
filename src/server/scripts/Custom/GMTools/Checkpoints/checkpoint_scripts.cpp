/*
 * GM test suite commands (T-048): .checkpoint, .kit and .gmsuite. The GMSuite addon only sends these commands;
 * .gmsuite sync answers it with addon messages. See README.md.
 */

#include "CheckpointMgr.h"
#include "Chat.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <string>

// GM tools block (docs/ID_RANGES.md); linked to role 197 (Gamemaster Commands) by
// sql/updates/master/auth/2026_10_31_claude_13_gmtools_commands.sql
static rbac::RBACPermissions const RBAC_PERM_COMMAND_CHECKPOINT = static_cast<rbac::RBACPermissions>(91000);
static rbac::RBACPermissions const RBAC_PERM_COMMAND_KIT = static_cast<rbac::RBACPermissions>(91001);
static rbac::RBACPermissions const RBAC_PERM_COMMAND_GMSUITE = static_cast<rbac::RBACPermissions>(91002);

// Addon message prefix the GMSuite addon registers.
static char const* const GMSUITE_ADDON_PREFIX = "GMSuite";

void AddCheckpointHeartseedSteps();

class gmtools_checkpoint_worldscript : public WorldScript
{
public:
    gmtools_checkpoint_worldscript() : WorldScript("gmtools_checkpoint_worldscript") { }

    void OnStartup() override
    {
        sCheckpointMgr->LoadData();
    }
};

class gmtools_checkpoint_commandscript : public CommandScript
{
public:
    gmtools_checkpoint_commandscript() : CommandScript("gmtools_checkpoint_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> checkpointCommandTable =
        {
            { "list",    &HandleCheckpointListCommand,    RBAC_PERM_COMMAND_CHECKPOINT, Trinity::ChatCommands::Console::Yes },
            { "explain", &HandleCheckpointExplainCommand, RBAC_PERM_COMMAND_CHECKPOINT, Trinity::ChatCommands::Console::Yes },
            { "apply",   &HandleCheckpointApplyCommand,   RBAC_PERM_COMMAND_CHECKPOINT, Trinity::ChatCommands::Console::No },
            { "reload",  &HandleReloadCommand,            RBAC_PERM_COMMAND_CHECKPOINT, Trinity::ChatCommands::Console::Yes },
        };
        static std::vector<ChatCommand> kitCommandTable =
        {
            { "list",    &HandleKitListCommand,    RBAC_PERM_COMMAND_KIT, Trinity::ChatCommands::Console::Yes },
            { "explain", &HandleKitExplainCommand, RBAC_PERM_COMMAND_KIT, Trinity::ChatCommands::Console::Yes },
            { "apply",   &HandleKitApplyCommand,   RBAC_PERM_COMMAND_KIT, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> gmsuiteCommandTable =
        {
            { "refresh", &HandleRefreshCommand, RBAC_PERM_COMMAND_GMSUITE, Trinity::ChatCommands::Console::No },
            { "sync",    &HandleSyncCommand,    RBAC_PERM_COMMAND_GMSUITE, Trinity::ChatCommands::Console::No },
            { "classspells", &HandleClassSpellsCommand, RBAC_PERM_COMMAND_GMSUITE, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "checkpoint", checkpointCommandTable, RBAC_PERM_COMMAND_CHECKPOINT, Trinity::ChatCommands::Console::No },
            { "kit",        kitCommandTable,        RBAC_PERM_COMMAND_KIT,        Trinity::ChatCommands::Console::No },
            { "gmsuite",    gmsuiteCommandTable,    RBAC_PERM_COMMAND_GMSUITE,    Trinity::ChatCommands::Console::No },
        };
        return commandTable;
    }

    // The selected player, or the GM when nothing (or no player) is selected.
    static Player* GetTarget(ChatHandler* handler)
    {
        if (Player* target = handler->getSelectedPlayer())
            return target;
        return handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
    }

    static std::string FirstArg(char const* args)
    {
        std::string text = args ? args : "";
        size_t start = text.find_first_not_of(' ');
        if (start == std::string::npos)
            return "";
        size_t end = text.find(' ', start);
        return text.substr(start, end == std::string::npos ? std::string::npos : end - start);
    }

    static bool HandleCheckpointListCommand(ChatHandler* handler, char const* /*args*/)
    {
        std::vector<Checkpoints::Checkpoint> checkpoints = sCheckpointMgr->GetCheckpoints();
        handler->PSendSysMessage("%u checkpoints (.checkpoint explain <name>, .checkpoint apply <name>):", uint32(checkpoints.size()));
        for (Checkpoints::Checkpoint const& checkpoint : checkpoints)
            handler->PSendSysMessage(" %s [%s] %s", checkpoint.Name.c_str(), checkpoint.Category.c_str(), checkpoint.Description.c_str());
        return true;
    }

    static bool HandleCheckpointExplainCommand(ChatHandler* handler, char const* args)
    {
        Checkpoints::Checkpoint checkpoint;
        if (!sCheckpointMgr->GetCheckpoint(FirstArg(args), checkpoint))
        {
            handler->SendSysMessage("Usage: .checkpoint explain <name>; see .checkpoint list");
            handler->SetSentErrorMessage(true);
            return false;
        }
        handler->PSendSysMessage("%s: %s", checkpoint.Name.c_str(), checkpoint.Description.c_str());
        for (std::string const& line : sCheckpointMgr->Explain(checkpoint))
            handler->PSendSysMessage(" %s", line.c_str());
        return true;
    }

    static bool HandleCheckpointApplyCommand(ChatHandler* handler, char const* args)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;
        Checkpoints::Checkpoint checkpoint;
        if (!sCheckpointMgr->GetCheckpoint(FirstArg(args), checkpoint))
        {
            handler->SendSysMessage("Usage: .checkpoint apply <name>; see .checkpoint list");
            handler->SetSentErrorMessage(true);
            return false;
        }
        if (target->IsInCombat() || !target->IsAlive())
        {
            handler->PSendSysMessage("%s must be alive and out of combat.", target->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        handler->PSendSysMessage("Applying checkpoint %s to %s:", checkpoint.Name.c_str(), target->GetName().c_str());
        uint32 failed = sCheckpointMgr->Apply(handler, target, checkpoint);
        handler->PSendSysMessage("Checkpoint %s applied (%u of %u steps failed).", checkpoint.Name.c_str(), failed, uint32(checkpoint.Steps.size()));
        return true;
    }

    static bool HandleReloadCommand(ChatHandler* handler, char const* /*args*/)
    {
        sCheckpointMgr->LoadData();
        handler->PSendSysMessage("Reloaded %u checkpoints and %u kits.", uint32(sCheckpointMgr->GetCheckpoints().size()),
            uint32(sCheckpointMgr->GetKits().size()));
        return true;
    }

    static bool HandleKitListCommand(ChatHandler* handler, char const* /*args*/)
    {
        std::vector<Checkpoints::Kit> kits = sCheckpointMgr->GetKits();
        handler->PSendSysMessage("%u kits (.kit explain <name>, .kit apply <name>):", uint32(kits.size()));
        for (Checkpoints::Kit const& kit : kits)
            handler->PSendSysMessage(" %s (levels %u-%u, %u items) %s", kit.Name.c_str(), uint32(kit.MinLevel), uint32(kit.MaxLevel),
                uint32(kit.Items.size()), kit.Description.c_str());
        return true;
    }

    static bool HandleKitExplainCommand(ChatHandler* handler, char const* args)
    {
        Checkpoints::Kit kit;
        if (!sCheckpointMgr->GetKit(FirstArg(args), kit))
        {
            handler->SendSysMessage("Usage: .kit explain <name>; see .kit list");
            handler->SetSentErrorMessage(true);
            return false;
        }
        handler->PSendSysMessage("%s: %s", kit.Name.c_str(), kit.Description.c_str());
        for (Checkpoints::KitItem const& item : kit.Items)
            handler->PSendSysMessage(" item %u x%u%s%s", item.Entry, item.Count, item.Equip ? ", equipped" : "",
                item.ClassMask ? (", classes 0x" + std::to_string(item.ClassMask)).c_str() : "");
        return true;
    }

    static bool HandleKitApplyCommand(ChatHandler* handler, char const* args)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;
        Checkpoints::Kit kit;
        if (!sCheckpointMgr->GetKit(FirstArg(args), kit))
        {
            handler->SendSysMessage("Usage: .kit apply <name>; see .kit list");
            handler->SetSentErrorMessage(true);
            return false;
        }
        std::string message;
        sCheckpointMgr->ApplyKit(target, kit, message);
        handler->PSendSysMessage("%s: %s.", target->GetName().c_str(), message.c_str());
        return true;
    }

    // Ability tester: everything back to ready, so an ability can be cast again at once.
    static bool HandleRefreshCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;
        if (!target->IsAlive())
            target->ResurrectPlayer(1.0f);
        target->RemoveAllSpellCooldown();
        target->SetFullHealth();
        for (uint8 power = 0; power < MAX_POWERS; ++power)
        {
            // Eclipse is a bar with two ends, not a pool; leave it.
            if (Powers(power) == POWER_ECLIPSE)
                continue;
            int32 maxPower = target->GetMaxPower(Powers(power));
            if (maxPower > 0)
                target->SetPower(Powers(power), maxPower);
        }
        handler->PSendSysMessage("%s: cooldowns reset, health and power full.", target->GetName().c_str());
        return true;
    }

    // The class_spells checkpoint step on its own: level-up and specialization spells, no talents.
    static bool HandleClassSpellsCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;
        target->UpdateSkillsForLevel();
        target->LearnSpecializationSpells();
        handler->PSendSysMessage("%s: class and specialization spells for level %u learned.", target->GetName().c_str(), uint32(target->GetLevel()));
        return true;
    }

    // Answers the GMSuite addon with the checkpoint and kit lists as addon messages:
    // "C\t<name>\t<category>\t<description>", "K\t<name>\t<min>-<max>\t<description>", then "E".
    static bool HandleSyncCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        auto send = [player](std::string message)
        {
            if (message.size() > 250)
                message.resize(250);
            WorldPacket data;
            ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, player, player, message, 0, "", DEFAULT_LOCALE, GMSUITE_ADDON_PREFIX);
            player->SendDirectMessage(&data);
        };

        std::vector<Checkpoints::Checkpoint> checkpoints = sCheckpointMgr->GetCheckpoints();
        std::vector<Checkpoints::Kit> kits = sCheckpointMgr->GetKits();
        for (Checkpoints::Checkpoint const& checkpoint : checkpoints)
            send("C\t" + checkpoint.Name + "\t" + checkpoint.Category + "\t" + checkpoint.Description);
        for (Checkpoints::Kit const& kit : kits)
            send("K\t" + kit.Name + "\t" + std::to_string(kit.MinLevel) + "-" + std::to_string(kit.MaxLevel) + "\t" + kit.Description);
        send("E");
        handler->PSendSysMessage("GMSuite: sent %u checkpoints and %u kits.", uint32(checkpoints.size()), uint32(kits.size()));
        return true;
    }
};

void AddSC_gmtools_checkpoints()
{
    AddCheckpointHeartseedSteps();
    new gmtools_checkpoint_worldscript();
    new gmtools_checkpoint_commandscript();
}
