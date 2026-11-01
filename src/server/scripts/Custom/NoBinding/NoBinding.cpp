#include "ScriptMgr.h"
#include "Chat.h"
#include "Config.h"
#include "Item.h"
#include "NoBinding.h"
#include "Player.h"
#include "RBAC.h"
#include "World.h"
#include "WorldSession.h"

#include <sstream>

namespace
{
    bool Status(ChatHandler* handler, char const*)
    {
        handler->PSendSysMessage("NoBinding: enabled=%u; protected IDs='%s'", uint32(NoBinding::Enabled()),
            sConfigMgr->GetStringDefault("NoBinding.ProtectedItemIds", "").c_str());
        return true;
    }

    bool Inspect(ChatHandler* handler, char const* args, bool bind)
    {
        uint32 slot;
        std::string extra;
        std::istringstream input(args ? args : "");
        if (!(input >> slot) || (input >> extra) || slot >= PLAYER_SLOT_END)
            return false;
        Player* player = handler->getSelectedPlayer();
        if (!player)
            player = handler->GetSession()->GetPlayer();
        Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, uint8(slot));
        if (!item)
        {
            handler->SendSysMessage("No item in that equipment/backpack/bank slot");
            handler->SetSentErrorMessage(true);
            return false;
        }
        if (bind)
        {
            item->SetBinding(true);
            item->SetState(ITEM_CHANGED, player);
        }
        handler->PSendSysMessage("NoBinding item: slot=%u; entry=%u; guid=%u; suppressed=%u; soulbound=%u; trade=%u; tooltip-bonding=%u; tooltip-flags=%u",
            slot, item->GetEntry(), item->GetGUID().GetCounter(), uint32(NoBinding::SkipBinding(*item)),
            uint32(item->IsSoulBound()), uint32(item->CanBeTraded()), NoBinding::TooltipBonding(item->GetTemplate()), NoBinding::TooltipFlags(item->GetTemplate()));
        return true;
    }
    bool Pickup(ChatHandler* handler, char const* args)
    {
        uint32 entry;
        std::string extra;
        std::istringstream input(args ? args : "");
        if (!(input >> entry) || (input >> extra))
            return false;
        // Unlike .additem, exercise normal pickup without the GM command's unbinding pass.
        bool added = handler->GetSession()->GetPlayer()->AddItem(entry, 1);
        handler->PSendSysMessage("NoBinding normal pickup: entry=%u; added=%u", entry, uint32(added));
        return added;
    }
    bool ItemInfo(ChatHandler* handler, char const* args) { return Inspect(handler, args, false); }
    bool Bind(ChatHandler* handler, char const* args) { return Inspect(handler, args, true); }
    bool Refresh(ChatHandler* handler, char const*)
    {
        Player* player = handler->getSelectedPlayer();
        if (!player)
            player = handler->GetSession()->GetPlayer();
        handler->PSendSysMessage("NoBinding: refreshed %u ordinary bound items", NoBinding::RefreshPlayer(player));
        NoBinding::SendHotfixInfo(player);
        return true;
    }

    class no_binding_commands : public CommandScript
    {
    public:
        no_binding_commands() : CommandScript("no_binding_commands") { }
        Trinity::ChatCommands::ChatCommandTable GetCommands() const override
        {
            using namespace Trinity::ChatCommands;
            static ChatCommandTable children =
            {
                { "status", &Status, rbac::RBAC_PERM_COMMAND_ADDITEM, Console::Yes },
                { "pickup", &Pickup, rbac::RBAC_PERM_COMMAND_ADDITEM, Console::No },
                { "item", &ItemInfo, rbac::RBAC_PERM_COMMAND_ADDITEM, Console::No },
                { "bind", &Bind, rbac::RBAC_PERM_COMMAND_ADDITEM, Console::No },
                { "refresh", &Refresh, rbac::RBAC_PERM_COMMAND_ADDITEM, Console::No }
            };
            return { { "nobinding", children, rbac::RBAC_PERM_COMMAND_ADDITEM, Console::Yes } };
        }
    };

    class no_binding_world : public WorldScript
    {
    public:
        no_binding_world() : WorldScript("no_binding_world") { }
        void OnConfigLoad(bool reload) override
        {
            if (!reload)
                return;
            for (auto const& session : sWorld->GetAllSessions())
                if (Player* player = session.second->GetPlayer())
                {
                    NoBinding::RefreshPlayer(player);
                    if (!session.second->IsBot())
                        NoBinding::SendHotfixInfo(player);
                }
        }
    };

    class no_binding_player : public PlayerScript
    {
    public:
        no_binding_player() : PlayerScript("no_binding_player") { }
        void OnLogin(Player* player) override
        {
            if (NoBinding::Enabled() && !player->GetSession()->IsBot() &&
                sConfigMgr->GetBoolDefault("NoBinding.IntroOnLogin", true))
                player->GetSession()->SendNotification("Gear is tradeable. Quest and progression items stay protected.");
        }
    };
}

void AddSC_no_binding()
{
    new no_binding_commands();
    new no_binding_world();
    new no_binding_player();
}
