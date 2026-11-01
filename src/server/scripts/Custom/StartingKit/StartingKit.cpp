#include "StartingKitPolicy.h"
#include "FirstLoginHooks.h"
#include "Chat.h"
#include "Config.h"
#include "Item.h"
#include "Log.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldSession.h"
#include <algorithm>

namespace
{
    constexpr uint32 ApprenticeRiding = 33388;
    constexpr uint32 StarterBag = 4497; // Heavy Brown Bag, ten general-purpose slots.

    void Grant(Player* player)
    {
        // Bot sessions are already identified before login; BotFactory owns their bags/riding.
        if (player->GetSession()->IsBot())
        {
            TC_LOG_DEBUG("playerbots", "Starting kit: skip first login for bot %u", player->GetGUID().GetCounter());
            return;
        }
        bool mount = sConfigMgr->GetBoolDefault("StartingKit.Mount.Enable", true);
        bool bags = sConfigMgr->GetBoolDefault("StartingKit.Bags.Enable", true);
        if (mount)
        {
            if (uint32 spell = StartingKit::MountForRace(player->GetRace()))
            {
                player->LearnSpell(ApprenticeRiding, false);
                if (player->GetPureSkillValue(SKILL_RIDING) < 75)
                    player->SetSkill(SKILL_RIDING, std::max<uint16>(1, player->GetSkillStep(SKILL_RIDING)), 75,
                        std::max<uint16>(75, player->GetPureMaxSkillValue(SKILL_RIDING)));
                player->LearnSpell(spell, false);
            }
        }
        if (bags)
            for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
            {
                if (player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    continue;
                uint16 dest = 0;
                if (player->CanEquipNewItem(slot, dest, StarterBag, false) == EQUIP_ERR_OK)
                    player->EquipNewItem(dest, StarterBag, true);
                else
                    TC_LOG_ERROR("entities.player", "Starting kit: cannot equip bag for player %u in slot %u", player->GetGUID().GetCounter(), slot);
            }
        if (mount || bags)
            player->GetSession()->SendNotification(mount && bags ? "Your starting kit is ready: four bag slots and a mount with Apprentice Riding." :
                mount ? "Your starting mount and Apprentice Riding are ready in your Mounts collection." : "Your starting bags are ready.");
    }

    bool Inspect(ChatHandler* handler, char const* args)
    {
        if (args && *args)
            return false;
        Player* player = handler->getSelectedPlayer();
        if (!player)
            player = handler->GetSession()->GetPlayer();
        uint32 spell = StartingKit::MountForRace(player->GetRace());
        handler->PSendSysMessage("Starting kit: mount=%u; bags=%u; bot=%u; race=%u; riding=%u; mount spell=%u; known=%u",
            uint32(sConfigMgr->GetBoolDefault("StartingKit.Mount.Enable", true)), uint32(sConfigMgr->GetBoolDefault("StartingKit.Bags.Enable", true)),
            uint32(player->GetSession()->IsBot()), uint32(player->GetRace()), player->GetPureSkillValue(SKILL_RIDING), spell, uint32(player->HasSpell(spell)));
        for (uint8 slot = INVENTORY_SLOT_BAG_START; slot < INVENTORY_SLOT_BAG_END; ++slot)
        {
            Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
            handler->PSendSysMessage("Starting bag slot %u: item=%u; size=%u", slot, item ? item->GetEntry() : 0, item ? item->GetTemplate()->ContainerSlots : 0);
        }
        return true;
    }

    class starting_kit_commandscript : public CommandScript
    {
    public:
        starting_kit_commandscript() : CommandScript("starting_kit_commandscript") { }
        std::vector<ChatCommand> GetCommands() const override
        {
            return { { "startingkit", &Inspect, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Trinity::ChatCommands::Console::No } };
        }
    };
}
void AddSC_starting_kit()
{
    if (!FirstLoginHooks::Register(&Grant))
        TC_LOG_ERROR("scripts", "Starting kit first-login callback registration failed");
    new starting_kit_commandscript();
}
