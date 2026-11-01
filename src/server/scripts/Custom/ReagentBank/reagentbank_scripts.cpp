/*
 * Reagent bank commands (T-043). Players: .reagent [list|deposit|take|sync]. GMs: .reagent inspect, .reagent add.
 */

#include "ReagentBank.h"
#include "../DetachedStorage/DetachedStorage.h"
#include "Chat.h"
#include "Item.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

#include <cstdlib>

static rbac::RBACPermissions const RBAC_PERM_COMMAND_REAGENT = static_cast<rbac::RBACPermissions>(93002);
static rbac::RBACPermissions const RBAC_PERM_COMMAND_REAGENT_GM = static_cast<rbac::RBACPermissions>(93003);
static char const* const REAGENT_ADDON_PREFIX = "ReagentBank";

namespace
{
    std::string LinkFor(uint32 itemEntry, Player* player)
    {
        Detached::Entry entry;
        entry.ItemEntry = itemEntry;
        return DetachedStorage::Link(entry, uint8(player->GetLevel()));
    }

    // An item id or a shift-clicked item link.
    uint32 ParseItem(ChatHandler* handler, char* args)
    {
        char* key = handler->extractKeyFromLink(args, "Hitem");
        return key ? uint32(atoul(key)) : 0;
    }

    char const* DepositMessage(ReagentBank::DepositResult result)
    {
        switch (result)
        {
            case ReagentBank::DepositResult::Disabled:    return "The reagent bank is disabled.";
            case ReagentBank::DepositResult::NotReagent:  return "That isn't a crafting reagent.";
            case ReagentBank::DepositResult::NotInBags:   return "Only items in your bags can go into the reagent bank.";
            case ReagentBank::DepositResult::NotStorable: return "Items with a time limit stay in your bags.";
            case ReagentBank::DepositResult::Full:        return "Your reagent bank is full.";
            case ReagentBank::DepositResult::InCombat:    return "Not in combat.";
            default:                                       return "";
        }
    }

    void ShowContents(ChatHandler* handler, Player* player)
    {
        std::vector<ReagentBank::Holding> contents = ReagentBank::Contents(player);
        if (contents.empty())
        {
            handler->PSendSysMessage("%s's reagent bank is empty.", player->GetName().c_str());
            return;
        }
        uint32 stacks = 0;
        for (ReagentBank::Holding const& holding : contents)
        {
            handler->PSendSysMessage("  %s x%u", LinkFor(holding.ItemEntry, player).c_str(), holding.Count);
            stacks += holding.Stacks;
        }
        handler->PSendSysMessage("%s's reagent bank: %u kinds of reagent in %u stacks.", player->GetName().c_str(), uint32(contents.size()), stacks);
    }
}

class reagentbank_commandscript : public CommandScript
{
public:
    reagentbank_commandscript() : CommandScript("reagentbank_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> reagentCommandTable =
        {
            { "",        &HandleListCommand,    RBAC_PERM_COMMAND_REAGENT,    Trinity::ChatCommands::Console::No },
            { "list",    &HandleListCommand,    RBAC_PERM_COMMAND_REAGENT,    Trinity::ChatCommands::Console::No },
            { "deposit", &HandleDepositCommand, RBAC_PERM_COMMAND_REAGENT,    Trinity::ChatCommands::Console::No },
            { "take",    &HandleTakeCommand,    RBAC_PERM_COMMAND_REAGENT,    Trinity::ChatCommands::Console::No },
            { "sync",    &HandleSyncCommand,    RBAC_PERM_COMMAND_REAGENT,    Trinity::ChatCommands::Console::No },
            { "inspect", &HandleInspectCommand, RBAC_PERM_COMMAND_REAGENT_GM, Trinity::ChatCommands::Console::No },
            { "add",     &HandleAddCommand,     RBAC_PERM_COMMAND_REAGENT_GM, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "reagent", reagentCommandTable, RBAC_PERM_COMMAND_REAGENT, Trinity::ChatCommands::Console::No },
        };
        return commandTable;
    }

    static bool HandleListCommand(ChatHandler* handler, char const* /*args*/)
    {
        ShowContents(handler, handler->GetSession()->GetPlayer());
        return true;
    }

    // .reagent deposit: every reagent in the bags. .reagent deposit <bag 0-4> <slot 1+>: one stack.
    static bool HandleDepositCommand(ChatHandler* handler, char const* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        std::string text = args ? args : "";
        if (text.empty())
        {
            uint32 stacks = 0;
            bool full = false;
            uint32 items = ReagentBank::DepositAll(player, stacks, full);
            if (!ReagentBank::Enabled() || player->IsInCombat())
                handler->SendSysMessage(DepositMessage(ReagentBank::Enabled() ? ReagentBank::DepositResult::InCombat : ReagentBank::DepositResult::Disabled));
            else if (!items)
                handler->SendSysMessage(full ? "Your reagent bank is full." : "No reagents in your bags.");
            else
                handler->PSendSysMessage("Deposited %u reagents from %u stacks.%s", items, stacks, full ? " The reagent bank is full; the rest stayed in your bags." : "");
            return true;
        }

        char* bagText = strtok(const_cast<char*>(text.c_str()), " ");
        char* slotText = strtok(nullptr, " ");
        if (!bagText || !slotText)
            return false;
        uint32 bag = uint32(atoul(bagText));
        uint32 slot = uint32(atoul(slotText));
        if (bag > 4 || slot < 1)
            return false;
        // Bag 0 is the backpack; client slots count from 1.
        Item* item = bag == 0
            ? player->GetItemByPos(INVENTORY_SLOT_BAG_0, uint8(INVENTORY_SLOT_ITEM_START + slot - 1))
            : player->GetItemByPos(uint8(INVENTORY_SLOT_BAG_START + bag - 1), uint8(slot - 1));
        if (!item)
        {
            handler->SendSysMessage("Nothing in that bag slot.");
            return true;
        }
        uint32 deposited = 0;
        uint32 entry = item->GetEntry();
        ReagentBank::DepositResult result = ReagentBank::Deposit(player, item, deposited);
        if (result == ReagentBank::DepositResult::Ok || result == ReagentBank::DepositResult::Partial)
            handler->PSendSysMessage("Deposited %s x%u.%s", LinkFor(entry, player).c_str(), deposited,
                result == ReagentBank::DepositResult::Partial ? " The reagent bank is full; the rest stayed in your bags." : "");
        else
            handler->SendSysMessage(DepositMessage(result));
        return true;
    }

    // .reagent take <item id or link> [count]: count defaults to one full stack.
    static bool HandleTakeCommand(ChatHandler* handler, char const* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!args || !*args)
            return false;
        std::string text = args;
        uint32 itemEntry = ParseItem(handler, const_cast<char*>(text.c_str()));
        ItemTemplate const* proto = itemEntry ? sObjectMgr->GetItemTemplate(itemEntry) : nullptr;
        if (!proto)
            return false;
        char* countText = strtok(nullptr, " ");
        uint32 count = countText ? uint32(atoul(countText)) : std::max<uint32>(1, proto->GetMaxStackSize());
        if (!count)
            return false;

        uint32 withdrawn = 0;
        switch (ReagentBank::Withdraw(player, itemEntry, count, withdrawn))
        {
            case ReagentBank::WithdrawResult::Ok:
                handler->PSendSysMessage("Took %s x%u from the reagent bank.", LinkFor(itemEntry, player).c_str(), withdrawn);
                break;
            case ReagentBank::WithdrawResult::Partial:
                handler->PSendSysMessage("Took %s x%u from the reagent bank; your bags are full.", LinkFor(itemEntry, player).c_str(), withdrawn);
                break;
            case ReagentBank::WithdrawResult::Disabled: handler->SendSysMessage("The reagent bank is disabled."); break;
            case ReagentBank::WithdrawResult::Empty:    handler->SendSysMessage("None of those in your reagent bank."); break;
            case ReagentBank::WithdrawResult::BagsFull: handler->SendSysMessage("Your bags are full."); break;
            case ReagentBank::WithdrawResult::InCombat: handler->SendSysMessage("Not in combat."); break;
        }
        return true;
    }

    // Answers the ReagentBank addon: "R\t<item entry>\t<count>" per kind of reagent, then "E".
    static bool HandleSyncCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        auto send = [player](std::string const& message)
        {
            WorldPacket data;
            ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, player, player, message, 0, "", DEFAULT_LOCALE, REAGENT_ADDON_PREFIX);
            player->SendDirectMessage(&data);
        };
        for (ReagentBank::Holding const& holding : ReagentBank::Contents(player))
            send("R\t" + std::to_string(holding.ItemEntry) + "\t" + std::to_string(holding.Count));
        send("E");
        return true;
    }

    // .reagent inspect: the selected player's bank, stack by stack.
    static bool HandleInspectCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->getSelectedPlayer();
        if (!player)
            player = handler->GetSession()->GetPlayer();
        handler->PSendSysMessage("Reagent bank: enabled=%u, reagents known=%s", uint32(ReagentBank::Enabled()),
            ReagentBank::IsReagent(2589) ? "yes (Linen Cloth is one)" : "no");
        for (Detached::Entry const& entry : sDetachedStorage->List(player, Detached::Kind::ReagentBank, 0))
            handler->PSendSysMessage("  slot %u: item guid %u, %s x%u", entry.Slot, entry.ItemGuid, LinkFor(entry.ItemEntry, player).c_str(), entry.Count);
        ShowContents(handler, player);
        return true;
    }

    // .reagent add <item id or link> <count>: new items straight into the selected player's bank.
    static bool HandleAddCommand(ChatHandler* handler, char const* args)
    {
        if (!args || !*args)
            return false;
        Player* player = handler->getSelectedPlayer();
        if (!player)
            player = handler->GetSession()->GetPlayer();
        std::string text = args;
        uint32 itemEntry = ParseItem(handler, const_cast<char*>(text.c_str()));
        char* countText = strtok(nullptr, " ");
        uint32 count = countText ? uint32(atoul(countText)) : 1;
        if (!itemEntry || !sObjectMgr->GetItemTemplate(itemEntry) || !count)
            return false;
        uint32 added = ReagentBank::Add(player, itemEntry, count);
        handler->PSendSysMessage("Added %s x%u to %s's reagent bank%s.", LinkFor(itemEntry, player).c_str(), added, player->GetName().c_str(),
            ReagentBank::IsReagent(itemEntry) ? "" : " (not a reagent: crafting won't need it, but it can be taken out)");
        return true;
    }
};

void AddSC_reagent_bank()
{
    ReagentBank::Register();
    new reagentbank_commandscript();
}
