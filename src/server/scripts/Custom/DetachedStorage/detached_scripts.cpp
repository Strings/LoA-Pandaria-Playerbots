/*
 * Detached item storage (T-018): keeps each character's stored items loaded while it is online.
 */

#include "DetachedStorage.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

class detached_storage_playerscript : public PlayerScript
{
public:
    detached_storage_playerscript() : PlayerScript("detached_storage_playerscript") { }

    void OnLogin(Player* player) override
    {
        if (!player->GetSession()->IsBot())
            sDetachedStorage->LoadPlayer(player);
    }

    void OnLogout(Player* player) override
    {
        sDetachedStorage->UnloadPlayer(player);
    }

    void OnDelete(ObjectGuid guid) override
    {
        sDetachedStorage->DeletePlayer(guid.GetCounter());
    }
};

void AddSC_detached_storage()
{
    new detached_storage_playerscript();
}
