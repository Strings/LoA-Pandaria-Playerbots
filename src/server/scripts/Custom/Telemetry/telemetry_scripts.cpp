/*
 * Player telemetry (T-004): script hooks and the .telemetry commands. See README.md.
 */

#include "Custom/Telemetry/TelemetryMgr.h"

#include "Chat.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "TelemetryHooks.h"
#include "WorldSession.h"

#include <algorithm>
#include <cctype>

// From the telemetry block (docs/ID_RANGES.md). Players control their own logging; GMs inspect it.
static rbac::RBACPermissions const RBAC_PERM_COMMAND_TELEMETRY = static_cast<rbac::RBACPermissions>(95000);
static rbac::RBACPermissions const RBAC_PERM_COMMAND_TELEMETRY_GM = static_cast<rbac::RBACPermissions>(95001);

namespace
{
    constexpr size_t MaxIntentLength = 32;
    constexpr size_t SnapshotChunk = 200;

    // Every handler needs a live session; this reports why there is none.
    Player* SessionPlayer(ChatHandler* handler, Player* player)
    {
        if (!sTelemetryMgr->Enabled())
        {
            handler->SendSysMessage("Telemetry is off on this realm (Telemetry.Enable).");
            return nullptr;
        }
        if (!player)
        {
            handler->SendSysMessage("No character selected.");
            return nullptr;
        }
        return player;
    }

    bool NoSession(ChatHandler* handler, Player* player)
    {
        handler->PSendSysMessage("No telemetry session for %s; one starts at login.", player->GetName().c_str());
        return true;
    }

    Player* Self(ChatHandler* handler)
    {
        return SessionPlayer(handler, handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr);
    }

    // GM commands act on the selected player, or on yourself.
    Player* Selected(ChatHandler* handler)
    {
        Player* player = handler->getSelectedPlayer();
        if (!player && handler->GetSession())
            player = handler->GetSession()->GetPlayer();
        return SessionPlayer(handler, player);
    }

    bool ValidIntent(std::string const& intent)
    {
        return !intent.empty() && intent.size() <= MaxIntentLength && std::all_of(intent.begin(), intent.end(), [](char c)
            {
                return std::islower(static_cast<unsigned char>(c)) || std::isdigit(static_cast<unsigned char>(c)) || c == '-' || c == '_';
            });
    }
}

static bool HandleTelemetryStatusCommand(ChatHandler* handler, char const* /*args*/)
{
    Player* player = Self(handler);
    if (!player)
        return true;
    std::string text;
    if (!sTelemetryMgr->Describe(player, text))
        return NoSession(handler, player);
    handler->PSendSysMessage("Telemetry: %s.", text.c_str());
    return true;
}

static bool HandleTelemetryPauseCommand(ChatHandler* handler, char const* /*args*/)
{
    Player* player = Self(handler);
    if (!player)
        return true;
    if (!sTelemetryMgr->SetManualPause(player, true))
        return NoSession(handler, player);
    handler->SendSysMessage("Telemetry paused. .telemetry resume logs again.");
    return true;
}

static bool HandleTelemetryResumeCommand(ChatHandler* handler, char const* /*args*/)
{
    Player* player = Self(handler);
    if (!player)
        return true;
    if (!sTelemetryMgr->SetManualPause(player, false))
        return NoSession(handler, player);
    handler->SendSysMessage("Telemetry resumed.");
    return true;
}

static bool HandleTelemetryDiscardCommand(ChatHandler* handler, char const* args)
{
    Player* player = Self(handler);
    if (!player)
        return true;
    int32 minutes = (args && *args) ? atoi(args) : 0;
    if (minutes <= 0)
    {
        handler->PSendSysMessage("Usage: .telemetry discard <minutes> (up to %u).", sTelemetryMgr->DiscardWindowMinutes());
        return true;
    }
    size_t dropped = 0;
    uint32 applied = 0;
    if (!sTelemetryMgr->Discard(player, uint32(minutes), dropped, applied))
        return NoSession(handler, player);
    handler->PSendSysMessage("Telemetry: dropped %u rows from the last %u minutes.", uint32(dropped), applied);
    if (applied < uint32(minutes))
        handler->PSendSysMessage("Only the last %u minutes can be discarded; older rows are already on disk.", applied);
    return true;
}

static bool HandleTelemetryTagCommand(ChatHandler* handler, char const* args)
{
    Player* player = Self(handler);
    if (!player)
        return true;
    std::string intent = (args && *args) ? args : "";
    intent.erase(0, intent.find_first_not_of(' '));
    intent.erase(intent.find_last_not_of(' ') + 1);
    if (!intent.empty() && !ValidIntent(intent))
    {
        handler->PSendSysMessage("An intent is one word of lowercase letters, digits, - or _, up to %u characters (e.g. hunt-healer).", uint32(MaxIntentLength));
        return true;
    }
    if (!sTelemetryMgr->Tag(player, intent))
        return NoSession(handler, player);
    if (intent.empty())
        handler->SendSysMessage("Telemetry intent cleared.");
    else
        handler->PSendSysMessage("Telemetry intent: %s (until .telemetry tag with no intent).", intent.c_str());
    return true;
}

static bool HandleTelemetrySnapshotCommand(ChatHandler* handler, char const* /*args*/)
{
    Player* player = Selected(handler);
    if (!player)
        return true;
    std::string json;
    if (!sTelemetryMgr->Snapshot(player, json))
        return NoSession(handler, player);
    handler->PSendSysMessage("Telemetry snapshot of %s (%u bytes):", player->GetName().c_str(), uint32(json.size()));
    for (size_t at = 0; at < json.size(); at += SnapshotChunk)
        handler->SendSysMessage(json.substr(at, SnapshotChunk).c_str());
    return true;
}

static bool HandleTelemetryFlushCommand(ChatHandler* handler, char const* /*args*/)
{
    Player* player = Selected(handler);
    if (!player)
        return true;
    size_t written = 0;
    if (!sTelemetryMgr->Flush(player, written))
        return NoSession(handler, player);
    handler->PSendSysMessage("Telemetry: wrote %u buffered rows for %s.", uint32(written), player->GetName().c_str());
    return true;
}

static bool HandleTelemetryTrackCommand(ChatHandler* handler, char const* args)
{
    Player* bot = Selected(handler);
    if (!bot)
        return true;
    std::string mode = (args && *args) ? args : "";
    if (mode != "on" && mode != "off")
    {
        handler->SendSysMessage("Usage: select a playerbot, then .telemetry track on|off.");
        return true;
    }
    if (!sTelemetryMgr->Track(bot, mode == "on"))
    {
        handler->PSendSysMessage("%s is not a playerbot; human players are logged from login.", bot->GetName().c_str());
        return true;
    }
    handler->PSendSysMessage(mode == "on" ? "Telemetry: logging %s as a scripted bot." : "Telemetry: stopped logging %s.",
        bot->GetName().c_str());
    return true;
}

class telemetry_playerscript : public PlayerScript
{
public:
    telemetry_playerscript() : PlayerScript("telemetry_playerscript") { }

    void OnLogin(Player* player) override { sTelemetryMgr->OnLogin(player); }
    void OnLogout(Player* player) override { sTelemetryMgr->OnLogout(player); }
    void OnUpdate(Player* player, uint32 /*diff*/) override { sTelemetryMgr->OnPlayerUpdate(player); }
    void OnSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override { sTelemetryMgr->OnCastDone(player, spell); }
    void OnTextEmote(Player* player, uint32 textEmote, uint32 /*emoteNum*/, ObjectGuid /*guid*/) override { sTelemetryMgr->OnEmote(player, textEmote); }
    void OnCreatureKill(Player* killer, Creature* killed) override { sTelemetryMgr->OnKill(killer, killed); }
    void OnPVPKill(Player* killer, Player* killed) override
    {
        sTelemetryMgr->OnKill(killer, killed);
        sTelemetryMgr->OnKilledBy(killer, killed);
    }
    void OnPlayerKilledByCreature(Creature* killer, Player* killed) override { sTelemetryMgr->OnKilledBy(killer, killed); }
    void OnUpdateZone(Player* player, uint32 newZone, uint32 newArea) override { sTelemetryMgr->OnZone(player, newZone, newArea); }

    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg) override { sTelemetryMgr->OnChat(player, type, lang, msg); }
    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg, Player* /*receiver*/) override { sTelemetryMgr->OnChat(player, type, lang, msg); }
    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg, Group* /*group*/) override { sTelemetryMgr->OnChat(player, type, lang, msg); }
    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg, Guild* /*guild*/) override { sTelemetryMgr->OnChat(player, type, lang, msg); }
    void OnChat(Player* player, uint32 type, uint32 lang, std::string& msg, Channel* /*channel*/) override { sTelemetryMgr->OnChat(player, type, lang, msg); }
};

// PlayerScripts are not registered as UnitScripts, so damage needs its own script.
class telemetry_unitscript : public UnitScript
{
public:
    telemetry_unitscript() : UnitScript("telemetry_unitscript") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override { sTelemetryMgr->OnDamage(attacker, victim, damage); }
};

class telemetry_worldscript : public WorldScript
{
public:
    telemetry_worldscript() : WorldScript("telemetry_worldscript") { }

    void OnConfigLoad(bool /*reload*/) override { sTelemetryMgr->LoadConfig(); }
    void OnUpdate(uint32 diff) override { sTelemetryMgr->OnWorldUpdate(diff); }
    void OnShutdown() override { sTelemetryMgr->OnShutdown(); }
};

class telemetry_commandscript : public CommandScript
{
public:
    telemetry_commandscript() : CommandScript("telemetry_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> telemetryCommandTable =
        {
            { "status",   &HandleTelemetryStatusCommand,   RBAC_PERM_COMMAND_TELEMETRY,    Trinity::ChatCommands::Console::No },
            { "pause",    &HandleTelemetryPauseCommand,    RBAC_PERM_COMMAND_TELEMETRY,    Trinity::ChatCommands::Console::No },
            { "resume",   &HandleTelemetryResumeCommand,   RBAC_PERM_COMMAND_TELEMETRY,    Trinity::ChatCommands::Console::No },
            { "discard",  &HandleTelemetryDiscardCommand,  RBAC_PERM_COMMAND_TELEMETRY,    Trinity::ChatCommands::Console::No },
            { "tag",      &HandleTelemetryTagCommand,      RBAC_PERM_COMMAND_TELEMETRY,    Trinity::ChatCommands::Console::No },
            { "snapshot", &HandleTelemetrySnapshotCommand, RBAC_PERM_COMMAND_TELEMETRY_GM, Trinity::ChatCommands::Console::No },
            { "flush",    &HandleTelemetryFlushCommand,    RBAC_PERM_COMMAND_TELEMETRY_GM, Trinity::ChatCommands::Console::No },
            { "track",    &HandleTelemetryTrackCommand,    RBAC_PERM_COMMAND_TELEMETRY_GM, Trinity::ChatCommands::Console::No },
            { "",         &HandleTelemetryStatusCommand,   RBAC_PERM_COMMAND_TELEMETRY,    Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "telemetry", telemetryCommandTable, RBAC_PERM_COMMAND_TELEMETRY, Trinity::ChatCommands::Console::No },
        };
        return commandTable;
    }
};

void AddSC_telemetry()
{
    new telemetry_playerscript();
    new telemetry_unitscript();
    new telemetry_worldscript();
    new telemetry_commandscript();

    TelemetryHooks::Handlers handlers;
    handlers.CastStart = [](Player* player, Spell const* spell) { sTelemetryMgr->OnCastStart(player, spell); };
    handlers.CastCancel = [](Player* player, Spell const* spell) { sTelemetryMgr->OnCastCancel(player, spell); };
    handlers.Command = [](Player* player, char const* text) { sTelemetryMgr->OnCommand(player, text); };
    handlers.LootOpen = [](Player* player, WorldObject const* source) { sTelemetryMgr->OnLootOpen(player, source); };
    handlers.LootRoll = [](Player* player, uint32 itemId, uint8 choice) { sTelemetryMgr->OnLootRoll(player, itemId, choice); };
    handlers.WorldState = [](Map* map, uint32 field, uint32 value) { sTelemetryMgr->OnObjective(map, "world_state", field, value); };
    handlers.BossState = [](Map* map, uint32 bossId, uint32 state) { sTelemetryMgr->OnObjective(map, "boss", bossId, state); };
    TelemetryHooks::Set(handlers);
}
