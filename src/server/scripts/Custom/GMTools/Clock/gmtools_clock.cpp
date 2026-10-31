/*
 * .clock (T-039): speed up or jump CustomClock, the clock custom systems read for their own cycles. Clients get
 * the shifted time too, so day and night follow it. See README.md.
 */

#include "Chat.h"
#include "Config.h"
#include "CustomClock.h"
#include "CustomClockRules.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "World.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <cstdlib>

// GM tools block (docs/ID_RANGES.md); linked to role 197 by sql/updates/master/auth/2026_10_31_claude_18_gmtools_t039.sql
static rbac::RBACPermissions const RBAC_PERM_COMMAND_CLOCK = static_cast<rbac::RBACPermissions>(91003);

// The client's normal day/night rate: one game minute per real second (Player::SendInitialPacketsBeforeAddToMap).
static float const CLIENT_BASE_TIME_SPEED = 0.01666667f;

namespace
{
    void BuildTimeSpeed(WorldPacket& data)
    {
        data.Initialize(SMSG_LOGIN_SETTIMESPEED, 20);
        time_t now = CustomClock::Now();
        data << uint32(0);
        data.AppendPackedTime(now);
        data << uint32(0);
        data.AppendPackedTime(now);
        data << float(CLIENT_BASE_TIME_SPEED * CustomClock::GetSpeed());
    }

    void SyncClients()
    {
        WorldPacket data;
        BuildTimeSpeed(data);
        sWorld->SendGlobalMessage(&data);
    }

    std::string Describe()
    {
        int64 ahead = CustomClock::GetAheadSeconds();
        char buffer[160];
        snprintf(buffer, sizeof(buffer), "Clock: speed x%.2f, %lld d %02lld:%02lld:%02lld ahead of wall time.",
            CustomClock::GetSpeed(), (long long)(ahead / 86400), (long long)(ahead % 86400 / 3600),
            (long long)(ahead % 3600 / 60), (long long)(ahead % 60));
        return buffer;
    }
}

class gmtools_clock_playerscript : public PlayerScript
{
public:
    gmtools_clock_playerscript() : PlayerScript("gmtools_clock_playerscript") { }

    // Login sends wall time; a shifted clock overrides it so the new client's sky matches.
    void OnLogin(Player* player) override
    {
        if (!CustomClock::IsShifted())
            return;

        WorldPacket data;
        BuildTimeSpeed(data);
        player->SendDirectMessage(&data);
    }
};

class gmtools_clock_commandscript : public CommandScript
{
public:
    gmtools_clock_commandscript() : CommandScript("gmtools_clock_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> clockCommandTable =
        {
            { "info",  &HandleInfoCommand,  RBAC_PERM_COMMAND_CLOCK, Trinity::ChatCommands::Console::Yes },
            { "speed", &HandleSpeedCommand, RBAC_PERM_COMMAND_CLOCK, Trinity::ChatCommands::Console::Yes },
            { "jump",  &HandleJumpCommand,  RBAC_PERM_COMMAND_CLOCK, Trinity::ChatCommands::Console::Yes },
            { "reset", &HandleResetCommand, RBAC_PERM_COMMAND_CLOCK, Trinity::ChatCommands::Console::Yes },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "clock", clockCommandTable, RBAC_PERM_COMMAND_CLOCK, Trinity::ChatCommands::Console::Yes },
        };
        return commandTable;
    }

    static bool HandleInfoCommand(ChatHandler* handler, char const* /*args*/)
    {
        handler->SendSysMessage(Describe().c_str());
        return true;
    }

    static bool HandleSpeedCommand(ChatHandler* handler, char const* args)
    {
        float maxSpeed = sConfigMgr->GetFloatDefault("GMTools.Clock.MaxSpeed", 3600.0f);
        char* end = nullptr;
        double speed = args ? std::strtod(args, &end) : 0.0;
        if (!args || end == args || (end && *end && *end != ' ') || !(speed > 0.0) || speed > maxSpeed)
        {
            handler->PSendSysMessage("Usage: .clock speed <multiplier>, above 0 and at most %.0f (GMTools.Clock.MaxSpeed).", maxSpeed);
            handler->SetSentErrorMessage(true);
            return false;
        }

        CustomClock::SetSpeed(speed);
        SyncClients();
        handler->SendSysMessage(Describe().c_str());
        return true;
    }

    static bool HandleJumpCommand(ChatHandler* handler, char const* args)
    {
        int64 maxJump = sConfigMgr->GetIntDefault("GMTools.Clock.MaxJumpSeconds", 2592000);
        int64_t seconds = 0;
        if (!CustomClock::ParseDuration(args, maxJump, seconds))
        {
            handler->PSendSysMessage("Usage: .clock jump <duration>, e.g. 90s, 15m, 6h, 1d12h; forward only, at most %lld seconds (GMTools.Clock.MaxJumpSeconds).", (long long)maxJump);
            handler->SetSentErrorMessage(true);
            return false;
        }

        CustomClock::Jump(seconds);
        SyncClients();
        handler->SendSysMessage(Describe().c_str());
        return true;
    }

    static bool HandleResetCommand(ChatHandler* handler, char const* /*args*/)
    {
        CustomClock::Reset();
        SyncClients();
        handler->SendSysMessage(Describe().c_str());
        return true;
    }
};

void AddSC_gmtools_clock()
{
    new gmtools_clock_playerscript();
    new gmtools_clock_commandscript();
}
