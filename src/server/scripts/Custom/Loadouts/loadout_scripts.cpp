/*
 * Spec loadouts (T-017): player hooks and the .loadout command. The Loadouts addon only sends these commands;
 * .loadout sync answers it with addon messages. See README.md.
 */

#include "LoadoutMgr.h"
#include "Chat.h"
#include "DBCStores.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <string>

// Loadouts block (docs/ID_RANGES.md). 92000 is linked to role 199 (Player Commands), 92001 to role 197
// (Gamemaster Commands) by sql/updates/master/auth/2026_11_01_claude_01_loadout_command.sql.
static rbac::RBACPermissions const RBAC_PERM_COMMAND_LOADOUT = static_cast<rbac::RBACPermissions>(92000);
static rbac::RBACPermissions const RBAC_PERM_COMMAND_LOADOUT_GM = static_cast<rbac::RBACPermissions>(92001);

// Addon message prefix the Loadouts addon registers.
static char const* const LOADOUT_ADDON_PREFIX = "Loadouts";

void AddLoadoutHeartseedPart();

namespace
{
    std::string SpecName(uint16 specId)
    {
        ChrSpecializationEntry const* entry = sChrSpecializationStore.LookupEntry(specId);
        return entry && entry->Name ? entry->Name : "?";
    }

    char const* SwitchMessage(Loadouts::SwitchResult result)
    {
        switch (result)
        {
            case Loadouts::SwitchResult::AlreadyActive:  return "That loadout is already active.";
            case Loadouts::SwitchResult::UnknownLoadout: return "No such loadout. Type .loadout to list them.";
            case Loadouts::SwitchResult::Disabled:       return "Loadouts are turned off.";
            case Loadouts::SwitchResult::NoSpecs:        return "Choose a specialization first.";
            case Loadouts::SwitchResult::NoSecondGroup:  return "Loadouts need a second talent group (dual specialization).";
            case Loadouts::SwitchResult::InCombat:       return "You can't change loadouts in combat.";
            case Loadouts::SwitchResult::Busy:           return "You can't change loadouts right now.";
            default:                                     return "";
        }
    }

    char const* EditMessage(Loadouts::EditResult result)
    {
        switch (result)
        {
            case Loadouts::EditResult::UnknownLoadout: return "No such loadout. Type .loadout to list them.";
            case Loadouts::EditResult::BadName:        return "Names are 1 to 24 characters, not only digits, with no | or tabs.";
            case Loadouts::EditResult::NameTaken:      return "You already have a loadout with that name.";
            case Loadouts::EditResult::TooMany:        return "You have the most named loadouts allowed. Delete one first.";
            case Loadouts::EditResult::IsDefault:      return "A specialization's own loadout can't be deleted.";
            case Loadouts::EditResult::IsActive:       return "One of your talent groups holds that loadout. Switch to two others first.";
            case Loadouts::EditResult::NoSpecs:        return "Choose a specialization first.";
            default:                                   return "";
        }
    }

    void SendList(ChatHandler* handler, Player* player)
    {
        uint32 active = sLoadoutMgr->GetActiveId(player);
        std::vector<Loadouts::Loadout> loadouts = sLoadoutMgr->GetLoadouts(player);
        if (loadouts.empty())
        {
            handler->PSendSysMessage("%s has no loadouts yet (they start when a specialization is chosen).", player->GetName().c_str());
            return;
        }
        handler->PSendSysMessage("Loadouts of %s:", player->GetName().c_str());
        for (Loadouts::Loadout const& loadout : loadouts)
        {
            std::string flags;
            if (loadout.Id == active)
                flags += " |cff00ff00(active)|r";
            else if (loadout.Group != Loadouts::NoGroup)
                flags += " (second group)";
            if (!loadout.Captured)
                flags += " (empty)";
            handler->PSendSysMessage("  %u. %s - %s, %u talents, %u buttons%s", loadout.Id, loadout.Name.c_str(),
                SpecName(loadout.SpecId).c_str(), uint32(loadout.Talents.size()), uint32(loadout.Actions.size()), flags.c_str());
        }
    }

    // Splits "<loadout> <new name>": the loadout is the first word unless the whole string names one.
    bool SplitTarget(Player* player, std::string const& args, uint32& id, std::string& rest)
    {
        std::size_t space = args.find(' ');
        if (space == std::string::npos)
            return false;
        id = sLoadoutMgr->Resolve(player, args.substr(0, space));
        rest = args.substr(space + 1);
        while (!rest.empty() && rest.front() == ' ')
            rest.erase(rest.begin());
        return id != 0;
    }
}

class loadout_playerscript : public PlayerScript
{
public:
    loadout_playerscript() : PlayerScript("loadout_playerscript") { }

    void OnLogin(Player* player) override
    {
        sLoadoutMgr->LoadPlayer(player);
        sLoadoutMgr->EnsureDefaults(player);
    }

    void OnSave(Player* player) override
    {
        sLoadoutMgr->SavePlayer(player);
    }

    void OnLogout(Player* player) override
    {
        sLoadoutMgr->SavePlayer(player);
        sLoadoutMgr->UnloadPlayer(player);
    }

    void OnDelete(ObjectGuid guid) override
    {
        sLoadoutMgr->DeletePlayer(guid.GetCounter());
    }

    void OnSpecializationChanged(Player* player) override
    {
        sLoadoutMgr->OnSpecializationChanged(player);
    }

    void OnLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        // The second talent group arrives with the specialization level (T-016).
        sLoadoutMgr->EnsureDefaults(player);
    }
};

class loadout_commandscript : public CommandScript
{
public:
    loadout_commandscript() : CommandScript("loadout_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> loadoutCommandTable =
        {
            { "",        &HandleListCommand,    RBAC_PERM_COMMAND_LOADOUT,    Trinity::ChatCommands::Console::No },
            { "list",    &HandleListCommand,    RBAC_PERM_COMMAND_LOADOUT,    Trinity::ChatCommands::Console::No },
            { "switch",  &HandleSwitchCommand,  RBAC_PERM_COMMAND_LOADOUT,    Trinity::ChatCommands::Console::No },
            { "new",     &HandleNewCommand,     RBAC_PERM_COMMAND_LOADOUT,    Trinity::ChatCommands::Console::No },
            { "rename",  &HandleRenameCommand,  RBAC_PERM_COMMAND_LOADOUT,    Trinity::ChatCommands::Console::No },
            { "delete",  &HandleDeleteCommand,  RBAC_PERM_COMMAND_LOADOUT,    Trinity::ChatCommands::Console::No },
            { "save",    &HandleSaveCommand,    RBAC_PERM_COMMAND_LOADOUT,    Trinity::ChatCommands::Console::No },
            { "sync",    &HandleSyncCommand,    RBAC_PERM_COMMAND_LOADOUT,    Trinity::ChatCommands::Console::No },
            { "inspect", &HandleInspectCommand, RBAC_PERM_COMMAND_LOADOUT_GM, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "loadout", loadoutCommandTable, RBAC_PERM_COMMAND_LOADOUT, Trinity::ChatCommands::Console::No },
        };
        return commandTable;
    }

    static bool HandleListCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        sLoadoutMgr->CaptureActive(player);
        SendList(handler, player);
        return true;
    }

    static bool HandleSwitchCommand(ChatHandler* handler, char const* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        std::string arg = args ? args : "";
        if (arg.empty())
        {
            handler->SendSysMessage("Usage: .loadout switch <id or name>");
            return false;
        }
        uint32 id = sLoadoutMgr->Resolve(player, arg);
        Loadouts::SwitchResult result = id ? sLoadoutMgr->Switch(player, id) : Loadouts::SwitchResult::UnknownLoadout;
        if (result != Loadouts::SwitchResult::Ok)
        {
            handler->SendSysMessage(SwitchMessage(result));
            return result == Loadouts::SwitchResult::AlreadyActive;
        }
        // Saved at once (talents, glyphs, bars and the loadout rows), so a crash can't split the switch.
        player->SaveToDB();
        for (Loadouts::Loadout const& loadout : sLoadoutMgr->GetLoadouts(player))
            if (loadout.Id == id)
                handler->PSendSysMessage("Switched to %s (%s).", loadout.Name.c_str(), SpecName(loadout.SpecId).c_str());
        return true;
    }

    static bool HandleNewCommand(ChatHandler* handler, char const* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        std::string name = args ? args : "";
        uint32 newId = 0;
        Loadouts::EditResult result = sLoadoutMgr->Create(player, name, newId);
        if (result != Loadouts::EditResult::Ok)
        {
            handler->SendSysMessage(EditMessage(result));
            return false;
        }
        sLoadoutMgr->SavePlayer(player);
        handler->PSendSysMessage("Saved your current talents, glyphs and bars as loadout %u, %s.", newId, name.c_str());
        return true;
    }

    static bool HandleRenameCommand(ChatHandler* handler, char const* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        uint32 id = 0;
        std::string name;
        if (!SplitTarget(player, args ? args : "", id, name))
        {
            handler->SendSysMessage("Usage: .loadout rename <id or name> <new name>");
            return false;
        }
        Loadouts::EditResult result = sLoadoutMgr->Rename(player, id, name);
        if (result != Loadouts::EditResult::Ok)
        {
            handler->SendSysMessage(EditMessage(result));
            return false;
        }
        sLoadoutMgr->SavePlayer(player);
        handler->PSendSysMessage("Loadout %u is now called %s.", id, name.c_str());
        return true;
    }

    static bool HandleDeleteCommand(ChatHandler* handler, char const* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        uint32 id = sLoadoutMgr->Resolve(player, args ? args : "");
        Loadouts::EditResult result = id ? sLoadoutMgr->Delete(player, id) : Loadouts::EditResult::UnknownLoadout;
        if (result != Loadouts::EditResult::Ok)
        {
            handler->SendSysMessage(EditMessage(result));
            return false;
        }
        sLoadoutMgr->SavePlayer(player);
        handler->PSendSysMessage("Deleted loadout %u.", id);
        return true;
    }

    static bool HandleSaveCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        sLoadoutMgr->SavePlayer(player);
        handler->SendSysMessage("Active loadout saved.");
        return true;
    }

    // Answers the Loadouts addon: "L\t<id>\t<name>\t<spec name>\t<flags>" per loadout, then "E".
    // Flags: a = active, g = held by the second talent group, d = a specialization's own, e = empty.
    static bool HandleSyncCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        auto send = [player](std::string message)
        {
            if (message.size() > 250)
                message.resize(250);
            WorldPacket data;
            ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, player, player, message, 0, "", DEFAULT_LOCALE, LOADOUT_ADDON_PREFIX);
            player->SendDirectMessage(&data);
        };

        sLoadoutMgr->CaptureActive(player);
        uint32 active = sLoadoutMgr->GetActiveId(player);
        for (Loadouts::Loadout const& loadout : sLoadoutMgr->GetLoadouts(player))
        {
            std::string flags;
            if (loadout.Id == active)
                flags += 'a';
            else if (loadout.Group != Loadouts::NoGroup)
                flags += 'g';
            if (loadout.IsDefault)
                flags += 'd';
            if (!loadout.Captured)
                flags += 'e';
            send("L\t" + std::to_string(loadout.Id) + "\t" + loadout.Name + "\t" + SpecName(loadout.SpecId) + "\t" + flags);
        }
        send("E");
        return true;
    }

    // GM view of the selected player's loadouts.
    static bool HandleInspectCommand(ChatHandler* handler, char const* args)
    {
        Player* target = nullptr;
        if (!handler->extractPlayerTarget(args && *args ? const_cast<char*>(args) : nullptr, &target) || !target)
        {
            handler->SendSysMessage("Usage: .loadout inspect [player] (online players only)");
            return false;
        }
        sLoadoutMgr->CaptureActive(target);
        SendList(handler, target);
        handler->PSendSysMessage("Talent groups: %u, active group %u (spec %s).", uint32(target->GetSpecsCount()),
            uint32(target->GetActiveSpec()), SpecName(uint16(target->GetTalentSpecialization())).c_str());
        return true;
    }
};

void AddSC_loadouts()
{
    AddLoadoutHeartseedPart();
    new loadout_playerscript();
    new loadout_commandscript();
}
