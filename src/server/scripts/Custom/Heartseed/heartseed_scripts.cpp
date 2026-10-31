/*
 * Heartseed hooks and GM commands (T-020). State lives in HeartseedMgr.
 */

#include "Chat.h"
#include "HeartseedMgr.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

#include <ctime>

// Heartseed block of docs/ID_RANGES.md; linked to role 197 by sql/updates/master/auth/2026_10_31_claude_02_heartseed_command.sql
static rbac::RBACPermissions const RBAC_PERM_COMMAND_HEARTSEED = static_cast<rbac::RBACPermissions>(90000);

class heartseed_worldscript : public WorldScript
{
public:
    heartseed_worldscript() : WorldScript("heartseed_worldscript") { }

    void OnConfigLoad(bool /*reload*/) override
    {
        sHeartseedMgr->LoadConfig();
    }

    void OnStartup() override
    {
        sHeartseedMgr->LoadData();
    }
};

class heartseed_playerscript : public PlayerScript
{
public:
    heartseed_playerscript() : PlayerScript("heartseed_playerscript") { }

    void OnLogin(Player* player) override
    {
        // Bots never get a Heartseed; skipping them saves four queries per bot login.
        if (player->GetSession()->IsBot())
            return;

        sHeartseedMgr->LoadPlayer(player);
        // The inventory was visualized before the state loaded, so send the glow now.
        sHeartseedMgr->RefreshGlow(player);
    }

    void OnSave(Player* player) override
    {
        sHeartseedMgr->SavePlayer(player);
    }

    void OnLogout(Player* player) override
    {
        sHeartseedMgr->SavePlayer(player);
        sHeartseedMgr->UnloadPlayer(player);
    }

    void OnLevelChanged(Player* player, uint8 /*oldLevel*/) override
    {
        if (sHeartseedMgr->HasHeartseed(player))
            sHeartseedMgr->RefreshGlow(player);
    }

    void OnVisibleItemEnchant(Player* player, uint8 slot, Item* /*item*/, uint16& tempEnchantVisual) override
    {
        // The Heartseed rides on the main-hand weapon (ranged weapons sit there too in MoP).
        if (slot == EQUIPMENT_SLOT_MAINHAND)
            tempEnchantVisual = sHeartseedMgr->GetGlowEnchant(player);
    }
};

class heartseed_commandscript : public CommandScript
{
public:
    heartseed_commandscript() : CommandScript("heartseed_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> heartseedCommandTable =
        {
            { "info",   &HandleInfoCommand,   RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "grant",  &HandleGrantCommand,  RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "revoke", &HandleRevokeCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "heartseed", heartseedCommandTable, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
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

    static bool HandleInfoCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;

        // What the client is sent for the main hand: low half the permanent enchant, high half the
        // temporary enchant or the Heartseed glow.
        uint32 visibleEnchants = target->GetUInt32Value(PLAYER_FIELD_VISIBLE_ITEMS + 1 + EQUIPMENT_SLOT_MAINHAND * 2);
        std::string visible = "Main-hand visible enchants: permanent " + std::to_string(visibleEnchants & 0xFFFF) +
            ", temporary/glow " + std::to_string(visibleEnchants >> 16) + ".";

        Heartseed::State state;
        if (!sHeartseedMgr->GetState(target, state))
        {
            handler->PSendSysMessage("%s has no Heartseed. %s", target->GetName().c_str(), visible.c_str());
            return true;
        }

        uint8 stage = sHeartseedMgr->GetStage(target);
        uint16 specId = uint16(target->GetTalentSpecialization());
        uint32 majors = 0;
        auto loadout = state.Loadouts.find(specId);
        if (loadout != state.Loadouts.end())
            for (uint32 trait : loadout->second)
                majors += trait ? 1 : 0;
        uint32 activeEssences = 0;
        for (auto const& essence : state.Essences)
            activeEssences += essence.second.second ? 1 : 0;

        handler->PSendSysMessage("Heartseed of %s: rank %u, Growth %u, stage %u (%s) at level %u.",
            target->GetName().c_str(), uint32(state.Rank), state.Growth, uint32(stage), sHeartseedMgr->GetStageName(stage).c_str(), uint32(target->GetLevel()));
        handler->PSendSysMessage("Spec %u: %u of %u majors chosen, %u loadouts saved. Essences: %u collected, %u active.",
            uint32(specId), majors, uint32(Heartseed::MaxMajorTiers), uint32(state.Loadouts.size()), uint32(state.Essences.size()), activeEssences);
        handler->PSendSysMessage("Glow enchant %u, dominant path %u. Story flags: %u set. Granted %u s ago.%s",
            uint32(sHeartseedMgr->GetGlowEnchant(target)), uint32(sHeartseedMgr->GetDominantPath(target)), uint32(state.StoryFlags.size()),
            uint32(time(nullptr)) - state.GrantedTime, sHeartseedMgr->IsEnabled() ? "" : " (Heartseed.Enabled = 0)");
        handler->SendSysMessage(visible.c_str());
        return true;
    }

    static bool HandleGrantCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;

        if (!sHeartseedMgr->Grant(target))
        {
            handler->PSendSysMessage("%s already has a Heartseed.", target->GetName().c_str());
            return true;
        }
        handler->PSendSysMessage("Granted a Heartseed to %s.", target->GetName().c_str());
        return true;
    }

    static bool HandleRevokeCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;

        if (!sHeartseedMgr->Revoke(target))
        {
            handler->PSendSysMessage("%s has no Heartseed.", target->GetName().c_str());
            return true;
        }
        handler->PSendSysMessage("Removed the Heartseed of %s, with its loadouts, essences and story flags.", target->GetName().c_str());
        return true;
    }
};

void AddSC_heartseed()
{
    new heartseed_worldscript();
    new heartseed_playerscript();
    new heartseed_commandscript();
}
