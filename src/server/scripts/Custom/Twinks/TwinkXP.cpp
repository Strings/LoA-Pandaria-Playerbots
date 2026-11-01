#include "TwinkXP.h"
#include "ExperienceRules.h"
#include "Chat.h"
#include "Config.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldSession.h"
namespace Twinks
{
    bool FreeXPEnabled() { return sConfigMgr->GetBoolDefault("Twinks.FreeXPToggle", true); }
    bool SetExperienceEnabled(Player* player, bool enabled)
    {
        if (!FreeXPEnabled())
            return false;
        if (player->HasFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN) == enabled)
        {
            if (enabled)
                player->RemoveFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN);
            else
                player->SetFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN);
            player->SaveToDB();
        }
        return true;
    }
}
namespace
{
    rbac::RBACPermissions const PlayerPermission = static_cast<rbac::RBACPermissions>(95000);
    rbac::RBACPermissions const TargetPermission = static_cast<rbac::RBACPermissions>(95001);
    bool HandleXP(ChatHandler* handler, char const* args)
    {
        auto request = Twinks::ParseXPRequest(args ? args : "");
        if (request == Twinks::XPRequest::Invalid)
        {
            handler->SendSysMessage("Use .xp on, .xp off or .xp status");
            handler->SetSentErrorMessage(true);
            return false;
        }
        Player* player = handler->GetSession()->GetPlayer();
        if (handler->HasPermission(TargetPermission))
            if (Player* selected = handler->getSelectedPlayer())
                player = selected;
        if (request != Twinks::XPRequest::Status && !Twinks::SetExperienceEnabled(player, request == Twinks::XPRequest::On))
        {
            handler->SendSysMessage("The free XP toggle is disabled");
            handler->SetSentErrorMessage(true);
            return false;
        }
        handler->PSendSysMessage("XP gain: %s; target: %s; free-toggle=%u; battleground queue: shared (native)",
            player->HasFlag(PLAYER_FIELD_PLAYER_FLAGS, PLAYER_FLAGS_NO_XP_GAIN) ? "off" : "on",
            player->GetName().c_str(), uint32(Twinks::FreeXPEnabled()));
        return true;
    }
    class twink_xp_commands : public CommandScript
    {
    public:
        twink_xp_commands() : CommandScript("twink_xp_commands") { }
        Trinity::ChatCommands::ChatCommandTable GetCommands() const override
        {
            return { { "xp", &HandleXP, PlayerPermission, Trinity::ChatCommands::Console::No } };
        }
    };
    class twink_xp_player : public PlayerScript
    {
    public:
        twink_xp_player() : PlayerScript("twink_xp_player") { }
        void OnLogin(Player* player) override
        {
            if (!player->GetSession()->IsBot() && Twinks::FreeXPEnabled() && sConfigMgr->GetBoolDefault("Twinks.IntroOnLogin", true))
                player->GetSession()->SendNotification("Use .xp on or .xp off anywhere for free. XP-off characters share battleground queues.");
        }
    };
}
void AddSC_twink_xp()
{
    new twink_xp_commands();
    new twink_xp_player();
}
