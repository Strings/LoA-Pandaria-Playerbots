/*
 * Spec loadouts (T-017): player hooks and the .loadout command. The Loadouts addon only sends these commands;
 * .loadout sync answers it with addon messages. See README.md.
 */

#include "LoadoutMgr.h"
#include "GearPages.h"
#include "GearPageRules.h"
#include "../DetachedStorage/DetachedStorage.h"
#include "Chat.h"
#include "Item.h"
#include "DBCStores.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <string>

// Loadouts block (docs/ID_RANGES.md). 93000 is linked to role 199 (Player Commands), 93001 to role 197
// (Gamemaster Commands) by sql/updates/master/auth/2026_11_01_claude_01_loadout_command.sql.
static rbac::RBACPermissions const RBAC_PERM_COMMAND_LOADOUT = static_cast<rbac::RBACPermissions>(93000);
static rbac::RBACPermissions const RBAC_PERM_COMMAND_LOADOUT_GM = static_cast<rbac::RBACPermissions>(93001);

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
            case Loadouts::EditResult::NotEmpty:       return "Its gear page still holds items. Take them first (.loadout page take).";
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

    char const* const SlotNames[GearPages::SlotCount] = { "Head", "Neck", "Shoulders", "Shirt", "Chest", "Waist", "Legs",
        "Feet", "Wrists", "Hands", "Finger 1", "Finger 2", "Trinket 1", "Trinket 2", "Back", "Main hand", "Off hand", "Ranged",
        "Tabard" };

    std::string SlotName(uint32 slot)
    {
        return slot < GearPages::SlotCount ? SlotNames[slot] : "Spare " + std::to_string(slot - GearPages::OverflowStart + 1);
    }

    // The addon's "item:..." string for a stored piece (the link without colour and name).
    std::string ItemString(Detached::Entry const& entry)
    {
        std::string link = DetachedStorage::Link(entry, 0);
        std::size_t start = link.find("item:");
        std::size_t end = link.find("|h", start);
        return start == std::string::npos ? "" : link.substr(start, end - start);
    }

    // Client bag numbers (0 backpack, 1-4 bags) and 1-based slots, as the addon and GetContainerItemInfo use them.
    Item* ClientBagItem(Player* player, uint32 bag, uint32 slot)
    {
        if (slot < 1 || bag > 4)
            return nullptr;
        if (bag == 0)
            return slot <= uint32(INVENTORY_SLOT_ITEM_END - INVENTORY_SLOT_ITEM_START)
                ? player->GetItemByPos(INVENTORY_SLOT_BAG_0, uint8(INVENTORY_SLOT_ITEM_START + slot - 1)) : nullptr;
        return slot <= 255 ? player->GetItemByPos(uint8(INVENTORY_SLOT_BAG_START + bag - 1), uint8(slot - 1)) : nullptr;
    }

    void SendPage(ChatHandler* handler, Player* player, Loadouts::Loadout const& loadout, bool always)
    {
        std::vector<Detached::Entry> entries = sDetachedStorage->List(player, Detached::Kind::GearPage, loadout.Id);
        if (entries.empty() && !always)
            return;
        handler->PSendSysMessage("Gear page of %u. %s:%s", loadout.Id, loadout.Name.c_str(),
            entries.empty() ? " empty (it wears whatever is equipped)" : "");
        for (Detached::Entry const& entry : entries)
            handler->PSendSysMessage("  %s: %s", SlotName(entry.Slot).c_str(), DetachedStorage::Link(entry, player->GetLevel()).c_str());
    }

    // Splits "<loadout> <new name>". Loadout names can have spaces, so the longest leading words that name a
    // loadout win ("PvP Arms Arena" renames "PvP Arms").
    bool SplitTarget(Player* player, std::string const& args, uint32& id, std::string& rest)
    {
        id = 0;
        for (std::size_t space = args.find(' '); space != std::string::npos; space = args.find(' ', space + 1))
        {
            uint32 found = sLoadoutMgr->Resolve(player, args.substr(0, space));
            std::size_t start = args.find_first_not_of(' ', space);
            if (found && start != std::string::npos)
            {
                id = found;
                rest = args.substr(start);
            }
        }
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
        static std::vector<ChatCommand> pageCommandTable =
        {
            { "",        &HandlePageCommand,     RBAC_PERM_COMMAND_LOADOUT, Trinity::ChatCommands::Console::No },
            { "put",     &HandlePagePutCommand,  RBAC_PERM_COMMAND_LOADOUT, Trinity::ChatCommands::Console::No },
            { "take",    &HandlePageTakeCommand, RBAC_PERM_COMMAND_LOADOUT, Trinity::ChatCommands::Console::No },
        };
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
            { "page",    pageCommandTable,      RBAC_PERM_COMMAND_LOADOUT,    Trinity::ChatCommands::Console::No },
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
            return true;   // handled; false would add "Incorrect syntax"
        }
        // Saved at once (talents, glyphs, bars and the loadout rows), so a crash can't split the switch.
        // SaveToDB doesn't run the OnSave hooks (only the periodic save does), so the loadouts save themselves.
        player->SaveToDB();
        sLoadoutMgr->SavePlayer(player);
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
            return true;   // handled; false would add "Incorrect syntax"
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
            return true;   // handled; false would add "Incorrect syntax"
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
            return true;   // handled; false would add "Incorrect syntax"
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

    // Answers the Loadouts addon: "L\t<id>\t<name>\t<spec name>\t<flags>" per loadout, each followed by
    // "G\t<id>\t<page slot>\t<item string>" per piece on its gear page, then "E".
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
            for (Detached::Entry const& entry : sDetachedStorage->List(player, Detached::Kind::GearPage, loadout.Id))
                send("G\t" + std::to_string(loadout.Id) + "\t" + std::to_string(entry.Slot) + "\t" + ItemString(entry));
        }
        send("E");
        return true;
    }

    // .loadout page [loadout]: one page, or every page with pieces on it.
    static bool HandlePageCommand(ChatHandler* handler, char const* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        std::string arg = args ? args : "";
        if (!arg.empty())
        {
            uint32 id = sLoadoutMgr->Resolve(player, arg);
            for (Loadouts::Loadout const& loadout : sLoadoutMgr->GetLoadouts(player))
                if (loadout.Id == id)
                {
                    SendPage(handler, player, loadout, true);
                    return true;
                }
            handler->SendSysMessage(EditMessage(Loadouts::EditResult::UnknownLoadout));
            return true;
        }
        bool any = false;
        for (Loadouts::Loadout const& loadout : sLoadoutMgr->GetLoadouts(player))
        {
            any = any || !sDetachedStorage->List(player, Detached::Kind::GearPage, loadout.Id).empty();
            SendPage(handler, player, loadout, false);
        }
        if (!any)
            handler->SendSysMessage("No gear pages hold anything yet. Put off-spec pieces on a loadout's page with "
                ".loadout page put <loadout> <bag> <slot>, or drop them on it in the Loadouts addon (/lo).");
        return true;
    }

    // .loadout page put <loadout> <bag> <slot> [equipment slot]: bag 0-4 and slot from 1, as the client counts them.
    static bool HandlePagePutCommand(ChatHandler* handler, char const* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        uint32 id = 0;
        std::string rest;
        uint32 bag = 0, bagSlot = 0, equipSlot = GearPages::NoSlot;
        if (!SplitTarget(player, args ? args : "", id, rest) || sscanf(rest.c_str(), "%u %u %u", &bag, &bagSlot, &equipSlot) < 2)
        {
            handler->SendSysMessage("Usage: .loadout page put <loadout> <bag 0-4> <slot> [equipment slot 0-18]");
            return true;
        }
        Item* item = ClientBagItem(player, bag, bagSlot);
        if (!item)
        {
            handler->SendSysMessage("There is no item in that bag slot.");
            return true;
        }
        uint8 slot = GearPages::NoSlot;
        switch (GearPages::Put(player, id, item, uint8(std::min<uint32>(equipSlot, GearPages::NoSlot)), slot))
        {
            case GearPages::PutResult::Ok:
                handler->PSendSysMessage("Put on the gear page (%s). It is equipped when you switch to that loadout.", SlotName(slot).c_str());
                break;
            case GearPages::PutResult::Disabled:       handler->SendSysMessage("Gear pages are turned off."); break;
            case GearPages::PutResult::UnknownLoadout: handler->SendSysMessage(EditMessage(Loadouts::EditResult::UnknownLoadout)); break;
            case GearPages::PutResult::WornPage:       handler->SendSysMessage("That is the loadout you are using: just equip the item."); break;
            case GearPages::PutResult::NotEquippable:  handler->SendSysMessage("That item doesn't go in that equipment slot."); break;
            case GearPages::PutResult::NotStorable:    handler->SendSysMessage("Timed items and bags with items in them can't go on a page."); break;
            case GearPages::PutResult::InCombat:       handler->SendSysMessage("You can't change gear pages in combat."); break;
        }
        return true;
    }

    // .loadout page take <loadout> <page slot>: back into the bags.
    static bool HandlePageTakeCommand(ChatHandler* handler, char const* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        uint32 id = 0;
        std::string rest;
        uint32 slot = 0;
        if (!SplitTarget(player, args ? args : "", id, rest) || sscanf(rest.c_str(), "%u", &slot) != 1)
        {
            handler->SendSysMessage("Usage: .loadout page take <loadout> <page slot> (0-18, or 100 and up for spares)");
            return true;
        }
        switch (GearPages::Take(player, id, slot))
        {
            case GearPages::TakeResult::Ok:             handler->PSendSysMessage("Moved the %s piece to your bags.", SlotName(slot).c_str()); break;
            case GearPages::TakeResult::Disabled:       handler->SendSysMessage("Gear pages are turned off."); break;
            case GearPages::TakeResult::UnknownLoadout: handler->SendSysMessage(EditMessage(Loadouts::EditResult::UnknownLoadout)); break;
            case GearPages::TakeResult::Empty:          handler->SendSysMessage("That page slot is empty."); break;
            case GearPages::TakeResult::BagsFull:       handler->SendSysMessage("Your bags are full."); break;
            case GearPages::TakeResult::InCombat:       handler->SendSysMessage("You can't change gear pages in combat."); break;
        }
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
        for (Loadouts::Loadout const& loadout : sLoadoutMgr->GetLoadouts(target))
            SendPage(handler, target, loadout, false);
        handler->PSendSysMessage("Talent groups: %u, active group %u (spec %s).", uint32(target->GetSpecsCount()),
            uint32(target->GetActiveSpec()), SpecName(uint16(target->GetTalentSpecialization())).c_str());
        return true;
    }
};

void AddSC_loadouts()
{
    AddLoadoutHeartseedPart();
    GearPages::Register();
    new loadout_playerscript();
    new loadout_commandscript();
}
