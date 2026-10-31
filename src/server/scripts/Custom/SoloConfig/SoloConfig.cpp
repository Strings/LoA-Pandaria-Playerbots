#include "Chat.h"
#include "DBCStores.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "SoloConfig.h"
#include "WorldSession.h"
#include <limits>
#include <sstream>

namespace
{
    Player* Target(ChatHandler* handler)
    {
        if (Player* player = handler->getSelectedPlayer())
            return player;
        return handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
    }

    bool Fail(ChatHandler* handler, char const* message)
    {
        handler->SendSysMessage(message);
        handler->SetSentErrorMessage(true);
        return false;
    }

    bool Info(ChatHandler* handler, char const* args)
    {
        Player* player = Target(handler);
        if (!player || (args && *args))
            return Fail(handler, "Usage: solo info (selected player, otherwise yourself)");
        handler->PSendSysMessage("Solo: weekly caps=%s; fishing always=%s; Valor cap=%u; arena cap=%u; rated cap=%u; fishing=%u/%u",
            SoloConfig::WeeklyCurrencyCapsEnabled() ? "on" : "off", SoloConfig::GuaranteedFishingSkillups() ? "on" : "off",
            player->GetCurrencyWeekCap(CURRENCY_TYPE_VALOR_POINTS, false),
            player->GetCurrencyWeekCap(CURRENCY_TYPE_CONQUEST_META_ARENA, false),
            player->GetCurrencyWeekCap(CURRENCY_TYPE_CONQUEST_META_RATED_BG, false),
            player->GetPureSkillValue(SKILL_FISHING), player->GetPureMaxSkillValue(SKILL_FISHING));
        return true;
    }

    bool Fishing(ChatHandler* handler, char const* args)
    {
        Player* player = Target(handler);
        if (!player || (args && *args))
            return Fail(handler, "Usage: solo fishskill (one normal fishing skill-up check)");
        uint32 before = player->GetPureSkillValue(SKILL_FISHING);
        if (!before)
            return Fail(handler, "Learn fishing first");
        bool gained = player->UpdateFishingSkill();
        handler->PSendSysMessage("Fishing: %u -> %u (max %u); gain=%s; guaranteed=%s", before,
            player->GetPureSkillValue(SKILL_FISHING), player->GetPureMaxSkillValue(SKILL_FISHING),
            gained ? "yes" : "no", SoloConfig::GuaranteedFishingSkillups() ? "on" : "off");
        return true;
    }

    bool Award(ChatHandler* handler, char const* args)
    {
        Player* player = Target(handler);
        std::istringstream input(args ? args : "");
        uint32 currency;
        int64 amount;
        std::string extra;
        if (!player || !(input >> currency >> amount) || (input >> extra) || amount <= 0 ||
            amount > std::numeric_limits<int32>::max() || !sCurrencyTypesStore.LookupEntry(currency))
            return Fail(handler, "Usage: solo award <currency id> <positive raw amount> (uses normal limits/multipliers)");
        uint32 before = player->GetCurrency(currency, true);
        player->ModifyCurrency(currency, int32(amount));
        handler->PSendSysMessage("Currency %u: raw total %u -> %u; week=%u; week cap=%u", currency, before,
            player->GetCurrency(currency, true), player->GetCurrencyOnWeek(currency, true), player->GetCurrencyWeekCap(currency, true));
        return true;
    }

    class solo_config_commandscript : public CommandScript
    {
    public:
        solo_config_commandscript() : CommandScript("solo_config_commandscript") { }

        std::vector<ChatCommand> GetCommands() const override
        {
            using Trinity::ChatCommands::Console;
            std::vector<ChatCommand> commands =
            {
                { "info", &Info, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Console::No },
                { "fishskill", &Fishing, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Console::No },
                { "award", &Award, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Console::No }
            };
            return { { "solo", commands, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Console::No } };
        }
    };
}

void AddSC_solo_config()
{
    new solo_config_commandscript();
}
