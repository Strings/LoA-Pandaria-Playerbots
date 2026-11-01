/*
 * .spectate (T-039): watch a bot (or any player) through its eyes while the GM stays invisible. The camera uses the
 * core's viewpoint (the far-sight field), which has no aura to clean it up, so this script owns the cleanup: a bot
 * logging out with a viewer attached would trip the core's shared-vision assert. See README.md.
 */

#include "Chat.h"
#include "Config.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

#include <mutex>
#include <unordered_map>

// GM tools block (docs/ID_RANGES.md); linked to role 197 by sql/updates/master/auth/2026_10_31_claude_18_gmtools_t039.sql
static rbac::RBACPermissions const RBAC_PERM_COMMAND_SPECTATE = static_cast<rbac::RBACPermissions>(91006);

namespace
{
    struct Watch
    {
        ObjectGuid Target;
        std::string TargetName;
        bool Attached = false;
        bool WasGMVisible = true;
        uint32 PendingMs = 0;
    };

    std::mutex WatchLock;
    std::unordered_map<ObjectGuid, Watch> Watches; // by spectator

    bool SameInstance(Player* a, Player* b)
    {
        return a->IsInWorld() && b->IsInWorld() && a->GetMapId() == b->GetMapId() && a->GetInstanceId() == b->GetInstanceId();
    }

    // Caller holds WatchLock. Detaches the camera if it is still on the target and restores GM visibility.
    void EndLocked(ObjectGuid spectatorGuid, Watch const& watch, char const* reason)
    {
        Player* spectator = ObjectAccessor::FindPlayer(spectatorGuid);
        if (!spectator)
            return;

        if (watch.Attached)
            if (Player* target = ObjectAccessor::FindPlayer(watch.Target))
                if (spectator->GetViewpoint() == target)
                    spectator->SetViewpoint(target, false);

        spectator->SetGMVisible(watch.WasGMVisible);
        if (reason)
            ChatHandler(spectator->GetSession()).PSendSysMessage("Spectate ended: %s.", reason);
    }

    void End(ObjectGuid spectatorGuid, char const* reason)
    {
        std::lock_guard<std::mutex> guard(WatchLock);
        auto itr = Watches.find(spectatorGuid);
        if (itr == Watches.end())
            return;
        EndLocked(spectatorGuid, itr->second, reason);
        Watches.erase(itr);
    }
}

class gmtools_spectate_worldscript : public WorldScript
{
public:
    gmtools_spectate_worldscript() : WorldScript("gmtools_spectate_worldscript") { }

    // Runs on the world thread before maps update: attach once the GM has arrived, end when the target is gone or
    // has left the GM's map.
    void OnUpdate(uint32 diff) override
    {
        std::lock_guard<std::mutex> guard(WatchLock);
        if (Watches.empty())
            return;

        uint32 attachTimeoutMs = sConfigMgr->GetIntDefault("GMTools.Spectate.AttachTimeoutMs", 10000);
        for (auto itr = Watches.begin(); itr != Watches.end();)
        {
            Player* spectator = ObjectAccessor::FindPlayer(itr->first);
            Player* target = ObjectAccessor::FindPlayer(itr->second.Target);
            Watch& watch = itr->second;
            char const* reason = nullptr;

            if (!spectator)
                reason = "you left the world";
            else if (!target || !target->IsInWorld())
                reason = "the target left the world";
            else if (target->IsBeingTeleportedFar())
                reason = "the target is changing maps";
            else if (spectator->IsBeingTeleported())
                ; // still travelling to the target
            else if (watch.Attached && !SameInstance(spectator, target))
                reason = "the target left your map";
            else if (!watch.Attached && SameInstance(spectator, target))
            {
                spectator->SetViewpoint(target, true);
                watch.Attached = true;
                ChatHandler(spectator->GetSession()).PSendSysMessage("Spectating %s. `.spectate stop` returns your camera.", watch.TargetName.c_str());
            }
            else if (!watch.Attached && (watch.PendingMs += diff) > attachTimeoutMs)
                reason = "could not reach the target";

            if (reason)
            {
                EndLocked(itr->first, watch, reason);
                itr = Watches.erase(itr);
            }
            else
                ++itr;
        }
    }
};

class gmtools_spectate_playerscript : public PlayerScript
{
public:
    gmtools_spectate_playerscript() : PlayerScript("gmtools_spectate_playerscript") { }

    // Runs before the player leaves the map: detach every camera on it now, while the object is still whole.
    void OnLogout(Player* player) override
    {
        std::lock_guard<std::mutex> guard(WatchLock);
        if (Watches.empty())
            return;

        for (auto itr = Watches.begin(); itr != Watches.end();)
        {
            if (itr->first == player->GetGUID())
            {
                EndLocked(itr->first, itr->second, nullptr);
                itr = Watches.erase(itr);
            }
            else if (itr->second.Target == player->GetGUID())
            {
                EndLocked(itr->first, itr->second, "the target logged out");
                itr = Watches.erase(itr);
            }
            else
                ++itr;
        }
    }
};

class gmtools_spectate_commandscript : public CommandScript
{
public:
    gmtools_spectate_commandscript() : CommandScript("gmtools_spectate_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> commandTable =
        {
            { "spectate", &HandleSpectateCommand, RBAC_PERM_COMMAND_SPECTATE, Trinity::ChatCommands::Console::No },
        };
        return commandTable;
    }

    static bool HandleSpectateCommand(ChatHandler* handler, char const* args)
    {
        Player* spectator = handler->GetSession()->GetPlayer();
        std::string name = args ? args : "";
        name.erase(0, name.find_first_not_of(' '));
        name.erase(name.find_last_not_of(' ') + 1);
        if (name == "stop")
            return HandleStopCommand(handler);

        Player* target = name.empty() ? handler->getSelectedPlayer() : ObjectAccessor::FindPlayerByName(name);
        if (!target || target == spectator || !target->IsInWorld())
        {
            handler->SendSysMessage("Usage: .spectate <player or bot name> (or select one); .spectate stop");
            handler->SetSentErrorMessage(true);
            return false;
        }

        Map* targetMap = target->GetMap();
        if (!SameInstance(spectator, target) && targetMap->Instanceable())
        {
            handler->PSendSysMessage("%s is inside an instance; enter it first, then spectate.", target->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        End(spectator->GetGUID(), nullptr);

        Watch watch;
        watch.Target = target->GetGUID();
        watch.TargetName = target->GetName();
        watch.WasGMVisible = spectator->isGMVisible();
        spectator->SetGMVisible(false);

        // A viewpoint must be on the viewer's own map; the update attaches it once the GM has arrived.
        if (!SameInstance(spectator, target))
            spectator->TeleportTo(target->GetMapId(), target->GetPositionX(), target->GetPositionY(), target->GetPositionZ() + 2.0f,
                target->GetOrientation());

        {
            std::lock_guard<std::mutex> guard(WatchLock);
            Watches[spectator->GetGUID()] = watch;
        }
        handler->PSendSysMessage("Spectate: joining %s (you are invisible).", watch.TargetName.c_str());
        return true;
    }

    static bool HandleStopCommand(ChatHandler* handler)
    {
        Player* spectator = handler->GetSession()->GetPlayer();
        {
            std::lock_guard<std::mutex> guard(WatchLock);
            if (!Watches.count(spectator->GetGUID()))
            {
                handler->SendSysMessage("You are not spectating anyone.");
                return true;
            }
        }
        End(spectator->GetGUID(), "stopped");
        return true;
    }
};

void AddSC_gmtools_spectate()
{
    new gmtools_spectate_worldscript();
    new gmtools_spectate_playerscript();
    new gmtools_spectate_commandscript();
}
