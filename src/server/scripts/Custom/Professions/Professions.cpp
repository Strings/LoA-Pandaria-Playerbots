#include "ProfessionRules.h"
#include "Chat.h"
#include "Config.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "World.h"
#include "WorldSession.h"

namespace
{
    uint32 Known(Player* player)
    {
        uint32 count = 0;
        for (auto const& profession : ProfessionRules::Primary)
            if (player->HasSkill(profession.Skill))
                ++count;
        return count;
    }
    void Refresh(Player* player)
    {
        player->SetFreePrimaryProfessions(uint16(ProfessionRules::FreeSlots(
            sWorld->getIntConfig(CONFIG_MAX_PRIMARY_TRADE_SKILL), Known(player))));
    }
    Player* Target(ChatHandler* handler)
    {
        Player* player = handler->getSelectedPlayer();
        return player ? player : handler->GetSession()->GetPlayer();
    }
    bool Inspect(ChatHandler* handler, char const*)
    {
        Player* player = Target(handler);
        handler->PSendSysMessage("Professions: configured=%u; known=%u; free=%u", sWorld->getIntConfig(CONFIG_MAX_PRIMARY_TRADE_SKILL), Known(player), player->GetFreePrimaryProfessionPoints());
        for (auto const& profession : ProfessionRules::Primary)
            if (player->HasSkill(profession.Skill))
                handler->PSendSysMessage("Profession skill=%u; rank=%u; max=%u", profession.Skill, player->GetPureSkillValue(profession.Skill), player->GetPureMaxSkillValue(profession.Skill));
        return true;
    }
    bool LearnAll(ChatHandler* handler, char const*)
    {
        if (sWorld->getIntConfig(CONFIG_MAX_PRIMARY_TRADE_SKILL) < ProfessionRules::Primary.size())
        {
            handler->SendSysMessage("Set MaxPrimaryTradeSkill to 11 before forcing all professions");
            handler->SetSentErrorMessage(true);
            return false;
        }
        Player* player = Target(handler);
        Refresh(player);
        for (auto const& profession : ProfessionRules::Primary)
            if (!player->HasSkill(profession.Skill))
                player->LearnSpell(profession.FirstSpell, false);
        Refresh(player);
        return Inspect(handler, "");
    }
    class profession_commands : public CommandScript
    {
    public:
        profession_commands() : CommandScript("profession_commands") { }
        Trinity::ChatCommands::ChatCommandTable GetCommands() const override
        {
            using namespace Trinity::ChatCommands;
            static ChatCommandTable commands =
            {
                { "status", &Inspect, rbac::RBAC_PERM_COMMAND_ADDITEM, Console::No },
                { "learnall", &LearnAll, rbac::RBAC_PERM_COMMAND_ADDITEM, Console::No }
            };
            return { { "professions", commands, rbac::RBAC_PERM_COMMAND_ADDITEM, Console::No } };
        }
    };
    class profession_world : public WorldScript
    {
    public:
        profession_world() : WorldScript("profession_world") { }
        void OnConfigLoad(bool reload) override
        {
            if (reload)
                for (auto const& session : sWorld->GetAllSessions())
                    if (Player* player = session.second->GetPlayer())
                        Refresh(player);
        }
    };
    class profession_player : public PlayerScript
    {
    public:
        profession_player() : PlayerScript("profession_player") { }
        void OnLogin(Player* player) override
        {
            Refresh(player);
            if (!player->GetSession()->IsBot() && sWorld->getIntConfig(CONFIG_MAX_PRIMARY_TRADE_SKILL) >= 11 &&
                sConfigMgr->GetBoolDefault("Professions.IntroOnLogin", true))
                player->GetSession()->SendNotification("All eleven professions are available. ProfessionBar opens extra professions beyond the spellbook's first two.");
        }
    };
}
void AddSC_professions()
{
    new profession_commands();
    new profession_world();
    new profession_player();
}
