#include "Chat.h"
#include "MountedSpeed.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

namespace
{
    bool Inspect(ChatHandler* handler, char const* args)
    {
        if (args && *args)
            return false;
        Player* player = handler->getSelectedPlayer();
        if (!player)
            player = handler->GetSession()->GetPlayer();
        player->UpdateSpeed(MOVE_RUN, true);
        player->UpdateSpeed(MOVE_FLIGHT, true);
        handler->PSendSysMessage("Mount speed: multiplier=%.2f; mounted=%u; taxi=%u; run=%.3f; flight=%.3f",
            MountedSpeed::Multiplier(), uint32(player->IsMounted()), uint32(player->IsInFlight()),
            player->GetSpeedRate(MOVE_RUN), player->GetSpeedRate(MOVE_FLIGHT));
        return true;
    }

    class mounted_speed_commandscript : public CommandScript
    {
    public:
        mounted_speed_commandscript() : CommandScript("mounted_speed_commandscript") { }

        std::vector<ChatCommand> GetCommands() const override
        {
            return { { "mountspeed", &Inspect, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Trinity::ChatCommands::Console::No } };
        }
    };
}

void AddSC_mounted_speed()
{
    new mounted_speed_commandscript();
}
