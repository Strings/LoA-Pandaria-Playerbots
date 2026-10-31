#include "Chat.h"
#include "DBCStores.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "SoloConfig.h"
#include "WorldSession.h"
#include "World.h"
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
        handler->PSendSysMessage("Player: all taxis=%u; reclaim PvE=%u; reclaim PvP=%u", uint32(player->isTaxiCheater()), player->GetCorpseReclaimDelay(false), player->GetCorpseReclaimDelay(true));
        handler->PSendSysMessage("AllFlightPaths=%u", uint32(sWorld->getBoolConfig(CONFIG_ALL_TAXI_PATHS)));
        handler->PSendSysMessage("SkillChance.Orange=%u", uint32(sWorld->getIntConfig(CONFIG_SKILL_CHANCE_ORANGE)));
        handler->PSendSysMessage("SkillChance.Yellow=%u", uint32(sWorld->getIntConfig(CONFIG_SKILL_CHANCE_YELLOW)));
        handler->PSendSysMessage("SkillChance.Green=%u", uint32(sWorld->getIntConfig(CONFIG_SKILL_CHANCE_GREEN)));
        handler->PSendSysMessage("SkillChance.Grey=%u", uint32(sWorld->getIntConfig(CONFIG_SKILL_CHANCE_GREY)));
        handler->PSendSysMessage("SkillChance.MiningSteps=%u", uint32(sWorld->getIntConfig(CONFIG_SKILL_CHANCE_MINING_STEPS)));
        handler->PSendSysMessage("SkillChance.SkinningSteps=%u", uint32(sWorld->getIntConfig(CONFIG_SKILL_CHANCE_SKINNING_STEPS)));
        handler->PSendSysMessage("SkillChance.Prospecting=%u", uint32(sWorld->getBoolConfig(CONFIG_SKILL_PROSPECTING)));
        handler->PSendSysMessage("SkillChance.Milling=%u", uint32(sWorld->getBoolConfig(CONFIG_SKILL_MILLING)));
        handler->PSendSysMessage("DurabilityLoss.InPvP=%u", uint32(sWorld->getBoolConfig(CONFIG_DURABILITY_LOSS_IN_PVP)));
        handler->PSendSysMessage("DurabilityLoss.OnDeath=%g", sWorld->getRate(RATE_DURABILITY_LOSS_ON_DEATH));
        handler->PSendSysMessage("Death.SicknessLevel=%u", uint32(sWorld->getIntConfig(CONFIG_DEATH_SICKNESS_LEVEL)));
        handler->PSendSysMessage("Death.CorpseReclaimDelay.PvP=%u", uint32(sWorld->getBoolConfig(CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVP)));
        handler->PSendSysMessage("Death.CorpseReclaimDelay.PvE=%u", uint32(sWorld->getBoolConfig(CONFIG_DEATH_CORPSE_RECLAIM_DELAY_PVE)));
        handler->PSendSysMessage("AllowTwoSide.Interaction.Calendar=%u", uint32(sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_CALENDAR)));
        handler->PSendSysMessage("AllowTwoSide.Interaction.Channel=%u", uint32(sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_CHANNEL)));
        handler->PSendSysMessage("AllowTwoSide.Interaction.Group=%u", uint32(sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GROUP)));
        handler->PSendSysMessage("AllowTwoSide.Interaction.Guild=%u", uint32(sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_GUILD)));
        handler->PSendSysMessage("AllowTwoSide.Interaction.Auction=%u", uint32(sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_AUCTION)));
        handler->PSendSysMessage("AllowTwoSide.Trade=%u", uint32(sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_TRADE)));
        handler->PSendSysMessage("AllowTwoSide.WhoList=%u", uint32(sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_WHO_LIST)));
        handler->PSendSysMessage("AllowTwoSide.Accounts=%u", uint32(sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_ACCOUNTS)));
        handler->PSendSysMessage("AllowTwoSide.AddFriend=%u", uint32(sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_ADD_FRIEND)));
        handler->PSendSysMessage("AllowTwoSide.Interaction.Mail=%u", uint32(sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_MAIL)));
        handler->PSendSysMessage("AllowTwoSide.Interaction.Chat=%u", uint32(sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_CHAT)));
        handler->PSendSysMessage("AllowTwoSide.Interaction.DungeonFinder=%u", uint32(sWorld->getBoolConfig(CONFIG_ALLOW_TWO_SIDE_INTERACTION_LFG)));
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
        uint32 before = player->GetCurrency(currency, false);
        player->ModifyCurrency(currency, int32(amount));
        handler->PSendSysMessage("Currency %u: raw total %u -> %u; week=%u; week cap=%u", currency, before,
            player->GetCurrency(currency, false), player->GetCurrencyOnWeek(currency, false), player->GetCurrencyWeekCap(currency, false));
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
