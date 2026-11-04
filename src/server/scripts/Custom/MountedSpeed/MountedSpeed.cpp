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
        float topFlight = baseMoveSpeed[MOVE_FLIGHT] * MountedSpeed::AdjustRate(MountedSpeed::TopFlightRate, MountedSpeed::Multiplier());
        handler->PSendSysMessage("Flight paths: %.1f yd/s (Taxi.SpeedOfTopFlight=%.2f; top player flying %.1f yd/s; stock %.0f)",
            MountedSpeed::TaxiSpeed(MountedSpeed::StockTaxiSpeed, baseMoveSpeed[MOVE_FLIGHT], MountedSpeed::Multiplier(),
                MountedSpeed::TaxiFactor()), MountedSpeed::TaxiFactor(), topFlight, MountedSpeed::StockTaxiSpeed);
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
