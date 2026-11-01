/*
 * Heartseed hooks and commands (T-020 to T-026). State lives in HeartseedMgr.
 */

#include "Bag.h"
#include "Chat.h"
#include "Custom/Reload/ReloadRegistry.h"
#include "HeartseedMgr.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <string>

// Heartseed block of docs/ID_RANGES.md; linked to role 197 by sql/updates/master/auth/2026_10_31_claude_02_heartseed_command.sql
static rbac::RBACPermissions const RBAC_PERM_COMMAND_HEARTSEED = static_cast<rbac::RBACPermissions>(90000);
// Player command '.seed'; linked to role 199 (Player Commands) by sql/updates/master/auth/2026_10_31_claude_11_seed_command.sql
static rbac::RBACPermissions const RBAC_PERM_COMMAND_SEED = static_cast<rbac::RBACPermissions>(90001);

// Addon message prefix the Heartseed addon registers (.seed sync).
static char const* const HEARTSEED_ADDON_PREFIX = "Heartseed";

static char const* GetPathName(Heartseed::Path path)
{
    switch (path)
    {
        case Heartseed::Path::Xuen:   return "Xuen";
        case Heartseed::Path::ChiJi:  return "Chi-Ji";
        case Heartseed::Path::Niuzao: return "Niuzao";
        case Heartseed::Path::Yulon:  return "Yu'lon";
        default:                      return "none";
    }
}

class heartseed_worldscript : public WorldScript
{
public:
    heartseed_worldscript() : WorldScript("heartseed_worldscript") { }

    void OnConfigLoad(bool /*reload*/) override
    {
        sHeartseedMgr->LoadConfig();
    }

    void OnStartup() override
    {
        auto result = CustomReload::Tables().Reload("heartseed");
        if (!result.Success)
            TC_LOG_ERROR("server", "Heartseed reload failed: %s", result.Message.c_str());
        sHeartseedMgr->LoadAccounts();
    }
};

class heartseed_playerscript : public PlayerScript
{
public:
    heartseed_playerscript() : PlayerScript("heartseed_playerscript") { }

    void OnLogin(Player* player) override
    {
        // Random bots never have a Heartseed and skip the queries; the player's own alts logged in as bots show
        // the account's seed and use it at their own cap, but earn nothing and get no story beats or prompts.
        if (!sHeartseedMgr->ShouldLoad(player))
            return;

        sHeartseedMgr->LoadPlayer(player);
        // The inventory was equipped before the state loaded, so apply traits and send the glow now.
        sHeartseedMgr->Refresh(player);
        if (player->GetSession()->IsBot())
            return;
        // The cap settings may have changed since the last session.
        sHeartseedMgr->ApplyRankUps(player);
        // Characters that were already in Pandaria when the hook arrived.
        sHeartseedMgr->OnMapChanged(player);
        // No Heartseed yet: offer the granting quest. A choice waiting: open it.
        sHeartseedMgr->ScheduleGrantOffer(player);
        sHeartseedMgr->ScheduleChoicePrompt(player);
    }

    void OnQuestAdded(Player* player, Quest const* quest) override
    {
        if (!player->GetSession()->IsBot())
            sHeartseedMgr->OnQuestAdded(player, quest);
    }

    void OnQuestRewarded(Player* player, Quest const* quest) override
    {
        if (!player->GetSession()->IsBot())
            sHeartseedMgr->OnQuestRewarded(player, quest);
    }

    void OnSave(Player* player) override
    {
        sHeartseedMgr->SavePlayer(player);
    }

    void OnLogout(Player* player) override
    {
        sHeartseedMgr->SavePlayer(player);
        sHeartseedMgr->UnloadPlayer(player);
    }

    void OnLevelChanged(Player* player, uint8 oldLevel) override
    {
        Heartseed::State state;
        if (!sHeartseedMgr->GetState(player, state))
            return;

        sHeartseedMgr->Refresh(player);
        if (player->GetSession()->IsBot())
            return;
        // A level raises the rank cap; Growth banked at the old cap is spent now.
        sHeartseedMgr->ApplyRankUps(player);

        // A tier that opens with this level and has no major yet: tell the player and open the choice.
        uint16 specId = uint16(player->GetTalentSpecialization());
        auto loadout = state.Loadouts.find(specId);
        for (uint8 tier = 0; tier < Heartseed::MaxMajorTiers; ++tier)
        {
            uint8 tierLevel = Heartseed::MajorTierLevel(tier);
            bool chosen = loadout != state.Loadouts.end() && loadout->second[tier];
            if (oldLevel < tierLevel && player->GetLevel() >= tierLevel && !chosen && sHeartseedMgr->TierHasMajors(tier))
            {
                ChatHandler(player->GetSession()).SendSysMessage("Your Heartseed stirs. A Celestial path waits for your choice.");
                sHeartseedMgr->SendChoiceMenu(player);
                break;
            }
        }
    }

    void OnVisibleItemEnchant(Player* player, uint8 slot, Item* /*item*/, uint16& tempEnchantVisual) override
    {
        // The Heartseed rides on the main-hand weapon (ranged weapons sit there too in MoP).
        if (slot == EQUIPMENT_SLOT_MAINHAND)
            tempEnchantVisual = sHeartseedMgr->GetGlowEnchant(player);
    }

    void OnVisibleItemChanged(Player* player, uint8 slot, Item* item) override
    {
        // Traits scale with the main-hand weapon. During login the inventory loads before the state; OnLogin
        // applies the traits then.
        if (slot == EQUIPMENT_SLOT_MAINHAND && player->IsInWorld())
        {
            sHeartseedMgr->RefreshTraits(player);
            // The weapon it replaced may be fed.
            if (!player->GetSession()->IsBot())
                sHeartseedMgr->OnMainHandChanged(player, item);
        }
    }

    void OnItemDestroyedByPlayer(Player* player, Item* item) override
    {
        if (!player->GetSession()->IsBot())
            sHeartseedMgr->OnItemDestroyedByPlayer(player, item);
    }

    void OnGossipSelect(Player* player, uint32 menuId, uint32 sender, uint32 action) override
    {
        if (menuId == Heartseed::GOSSIP_MENU_FEED)
            sHeartseedMgr->OnFeedGossipSelect(player, action);
        else if (menuId == Heartseed::GOSSIP_MENU_CHOICE)
            sHeartseedMgr->OnChoiceGossipSelect(player, sender, action);
    }

    void OnMapChanged(Player* player) override
    {
        if (!player->GetSession()->IsBot())
            sHeartseedMgr->OnMapChanged(player);
    }

    void OnKillCredit(Player* player, Creature* killed) override
    {
        if (!player->GetSession()->IsBot())
            sHeartseedMgr->OnKillCredit(player, killed);
    }

    void OnDungeonCompleted(Player* player, uint32 /*dungeonId*/) override
    {
        if (!player->GetSession()->IsBot())
            sHeartseedMgr->OnDungeonCompleted(player);
    }

    void OnSpecializationChanged(Player* player) override
    {
        // Each specialization has its own loadout, and the glow follows its dominant path.
        sHeartseedMgr->Refresh(player);
    }
};

class heartseed_unitscript : public UnitScript
{
public:
    heartseed_unitscript() : UnitScript("heartseed_unitscript") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!damage || !attacker || !victim || attacker->GetTypeId() != TYPEID_PLAYER)
            return;
        // Alts logged in as bots proc too; a character without a Heartseed returns at the first lookup.
        sHeartseedMgr->OnDamageDealt(attacker->ToPlayer(), victim);
    }
};

class heartseed_commandscript : public CommandScript
{
public:
    heartseed_commandscript() : CommandScript("heartseed_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> flagCommandTable =
        {
            { "list",  &HandleFlagListCommand,  RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "set",   &HandleFlagSetCommand,   RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "clear", &HandleFlagClearCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> essenceCommandTable =
        {
            { "list",   &HandleEssenceListCommand,   RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "add",    &HandleEssenceAddCommand,    RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "remove", &HandleEssenceRemoveCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "slot",   &HandleEssenceSlotCommand,   RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> feedCommandTable =
        {
            { "preview", &HandleFeedPreviewCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "",        &HandleFeedCommand,        RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> heartseedCommandTable =
        {
            { "info",   &HandleInfoCommand,   RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "grant",  &HandleGrantCommand,  RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "revoke", &HandleRevokeCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "offer",  &HandleOfferCommand,  RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "rank",   &HandleRankCommand,   RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "growth", &HandleGrowthCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "flag",   flagCommandTable,     RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "cap",    &HandleCapCommand,    RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "feed",   feedCommandTable,     RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "essence", essenceCommandTable, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "traits", &HandleTraitsCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "major",  &HandleMajorCommand,  RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "reload", &HandleReloadCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "shrines", &HandleShrinesCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> seedCommandTable =
        {
            { "sync", &HandleSeedSyncCommand, RBAC_PERM_COMMAND_SEED, Trinity::ChatCommands::Console::No },
            { "",     &HandleSeedCommand,     RBAC_PERM_COMMAND_SEED, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "heartseed", heartseedCommandTable, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "seed",      seedCommandTable,      RBAC_PERM_COMMAND_SEED,      Trinity::ChatCommands::Console::No },
        };
        return commandTable;
    }

    // The selected player, or the GM when nothing (or no player) is selected.
    static Player* GetTarget(ChatHandler* handler)
    {
        if (Player* target = handler->getSelectedPlayer())
            return target;
        return handler->GetSession() ? handler->GetSession()->GetPlayer() : nullptr;
    }

    static bool HandleInfoCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;

        // What the client is sent for the main hand: low half the permanent enchant, high half the
        // temporary enchant or the Heartseed glow.
        uint32 visibleEnchants = target->GetUInt32Value(PLAYER_FIELD_VISIBLE_ITEMS + 1 + EQUIPMENT_SLOT_MAINHAND * 2);
        std::string visible = "Main-hand visible enchants: permanent " + std::to_string(visibleEnchants & 0xFFFF) +
            ", temporary/glow " + std::to_string(visibleEnchants >> 16) + ".";

        Heartseed::State state;
        if (!sHeartseedMgr->GetState(target, state))
        {
            handler->PSendSysMessage("%s has no Heartseed. %s", target->GetName().c_str(), visible.c_str());
            return true;
        }

        uint8 stage = sHeartseedMgr->GetStage(target);
        uint16 specId = uint16(target->GetTalentSpecialization());
        uint32 majors = 0;
        auto loadout = state.Loadouts.find(specId);
        if (loadout != state.Loadouts.end())
            for (uint32 trait : loadout->second)
                majors += trait ? 1 : 0;
        uint32 activeEssences = 0;
        uint8 openSlots = Heartseed::EssenceSlotsForLevel(target->GetLevel());
        for (auto const& essence : state.Essences)
            activeEssences += essence.second.second && essence.second.second <= openSlots ? 1 : 0;

        handler->PSendSysMessage("Heartseed of %s (account %u): rank %u, account rank %u, cap %u. Growth %u of %u for the next rank, stage %u (%s) at level %u. %u weapons fed.",
            target->GetName().c_str(), state.AccountId, uint32(sHeartseedMgr->GetEffectiveRank(target)), uint32(state.Rank),
            uint32(sHeartseedMgr->GetRankCap(target)), state.Growth, sHeartseedMgr->GetGrowthForNextRank(state.Rank), uint32(stage),
            sHeartseedMgr->GetStageName(stage).c_str(), uint32(target->GetLevel()), state.FedWeapons);
        handler->PSendSysMessage("Spec %u: %u of %u majors chosen, %u loadouts saved. Essences: %u collected, %u active.",
            uint32(specId), majors, uint32(Heartseed::MaxMajorTiers), uint32(state.Loadouts.size()), uint32(state.Essences.size()), activeEssences);
        Heartseed::Path dominant = sHeartseedMgr->GetDominantPath(target);
        handler->PSendSysMessage("Glow enchant %u, dominant path %u (%s). Story flags: %s. Granted %u s ago.%s",
            uint32(sHeartseedMgr->GetGlowEnchant(target)), uint32(dominant), GetPathName(dominant), DescribeFlags(state).c_str(),
            uint32(time(nullptr)) - state.GrantedTime, sHeartseedMgr->IsEnabled() ? "" : " (Heartseed.Enabled = 0)");
        handler->PSendSysMessage("Loadout: %s.", DescribeLoadout(state, specId).c_str());

        std::string auras;
        for (auto const& [spellId, amount] : state.AppliedAuras)
        {
            if (!auras.empty())
                auras += ", ";
            auras += std::to_string(spellId) + "=" + std::to_string(amount);
        }
        handler->PSendSysMessage("Applied auras (spell=amount; %u is the core weapon bonus): %s. Core proc: spell %u for %u damage, %u procs since login.",
            Heartseed::SPELL_CORE_WEAPON_BONUS, auras.empty() ? "none" : auras.c_str(), sHeartseedMgr->GetProcSpellId(),
            sHeartseedMgr->GetProcDamage(target), state.ProcCount);
        handler->PSendSysMessage("Essences: %s. %u essence procs since login.", DescribeEssences(target, state).c_str(), state.EssenceProcCount);
        handler->SendSysMessage(visible.c_str());
        return true;
    }

    static std::string DescribeLoadout(Heartseed::State const& state, uint16 specId)
    {
        auto loadout = state.Loadouts.find(specId);
        std::string text;
        for (uint8 tier = 0; tier < Heartseed::MaxMajorTiers; ++tier)
        {
            uint32 traitId = loadout != state.Loadouts.end() ? loadout->second[tier] : 0;
            if (!traitId)
                continue;
            Heartseed::TraitInfo trait;
            if (!text.empty())
                text += ", ";
            text += "level " + std::to_string(Heartseed::MajorTierLevel(tier)) + " " + std::to_string(traitId) + " " +
                (sHeartseedMgr->GetTrait(traitId, trait) ? trait.Name : std::string("(unknown trait)"));
        }
        return text.empty() ? "no majors chosen" : text;
    }

    static bool HandleTraitsCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = GetTarget(handler);
        Heartseed::State state;
        bool hasState = target && sHeartseedMgr->GetState(target, state);

        std::vector<Heartseed::TraitInfo> traits = sHeartseedMgr->GetTraits();
        handler->PSendSysMessage("%u Heartseed traits (* = active on %s):", uint32(traits.size()), hasState ? target->GetName().c_str() : "nobody");
        for (Heartseed::TraitInfo const& trait : traits)
        {
            bool active = hasState && state.AppliedAuras.count(trait.SpellId);
            std::string when = trait.Kind == Heartseed::TraitKind::Major
                ? "major, level " + std::to_string(Heartseed::MajorTierLevel(trait.Tier))
                : "minor, rank " + std::to_string(trait.MinRank);
            handler->PSendSysMessage("%s%u %s [%s, %s] spell %u, %d + %.3f per item level: %s", active ? "*" : " ", trait.Id, trait.Name.c_str(),
                GetPathName(trait.TraitPath), when.c_str(), trait.SpellId, trait.BaseAmount, trait.AmountPerItemLevel, trait.Description.c_str());
        }
        return true;
    }

    static bool HandleMajorCommand(ChatHandler* handler, char const* args)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;

        // ".heartseed major <tier level> <trait id | 0>"; the tier is named by its level (10, 25, ...).
        char* end = nullptr;
        long level = args ? strtol(args, &end, 10) : 0;
        uint8 tier = level > 0 && level < 256 ? Heartseed::MajorTierForLevel(uint8(level)) : Heartseed::MaxMajorTiers;
        long traitId = end && end != args ? strtol(end, &end, 10) : -1;
        if (tier >= Heartseed::MaxMajorTiers || traitId < 0)
        {
            handler->SendSysMessage("Usage: .heartseed major <10|25|40|55|70|85> <trait id, or 0 to clear>; see .heartseed traits");
            handler->SetSentErrorMessage(true);
            return false;
        }

        // GM choices skip the level check, like .heartseed rank skips the level cap.
        switch (sHeartseedMgr->ChooseMajor(target, tier, uint32(traitId), true))
        {
            case Heartseed::ChoiceResult::Ok:
                handler->PSendSysMessage("%s: level %ld major is now %ld for spec %u.", target->GetName().c_str(), level, traitId,
                    uint32(target->GetTalentSpecialization()));
                return true;
            case Heartseed::ChoiceResult::NoHeartseed:
                handler->PSendSysMessage("%s has no Heartseed.", target->GetName().c_str());
                return true;
            case Heartseed::ChoiceResult::UnknownTrait:
                handler->PSendSysMessage("Trait %ld does not exist.", traitId);
                break;
            case Heartseed::ChoiceResult::NotMajor:
                handler->PSendSysMessage("Trait %ld is a minor; minors fill in by rank.", traitId);
                break;
            case Heartseed::ChoiceResult::WrongTier:
                handler->PSendSysMessage("Trait %ld is not a level %ld major.", traitId, level);
                break;
            case Heartseed::ChoiceResult::TierLocked:
                handler->PSendSysMessage("%s is below level %ld.", target->GetName().c_str(), level);
                break;
        }
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleReloadCommand(ChatHandler* handler, char const* /*args*/)
    {
        // Stages, glows, traits and essences. New spells in spell_dbc still need a restart.
        auto result = CustomReload::Tables().Reload("heartseed");
        if (!result.Success)
        {
            handler->SendSysMessage(result.Message.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }
        Player* target = GetTarget(handler);
        if (target && sHeartseedMgr->HasHeartseed(target))
            sHeartseedMgr->Refresh(target);
        handler->SendSysMessage("Heartseed stages, glows, traits, essences, twink items and shrines reloaded. Other characters pick up trait changes at their next refresh (relog, level, weapon or spec change).");
        return true;
    }

    static std::string DescribeFlags(Heartseed::State const& state)
    {
        if (state.StoryFlags.empty())
            return "none";

        std::string text;
        for (auto const& [flag, setTime] : state.StoryFlags)
        {
            if (!text.empty())
                text += ", ";
            text += std::to_string(flag) + " " + HeartseedMgr::GetStoryFlagName(flag);
        }
        return text;
    }

    // A known flag name or a positive number; 0 if neither.
    static uint16 ParseFlag(char const* args)
    {
        if (!args || !*args)
            return 0;

        std::string text = args;
        text = text.substr(0, text.find(' '));
        if (uint16 flag = HeartseedMgr::GetStoryFlagByName(text))
            return flag;

        char* end = nullptr;
        unsigned long value = strtoul(text.c_str(), &end, 10);
        return (end && !*end && value > 0 && value <= 0xFFFF) ? uint16(value) : 0;
    }

    static bool HandleRankCommand(ChatHandler* handler, char const* args)
    {
        Player* target = GetTarget(handler);
        if (!target || !args || !*args)
            return false;

        char* end = nullptr;
        long rank = strtol(args, &end, 10);
        if (end == args || rank < 1 || rank > 255)
        {
            handler->SendSysMessage("Usage: .heartseed rank <1-255>");
            handler->SetSentErrorMessage(true);
            return false;
        }

        // GM commands may go past the level cap (T-022), so states can be tested without leveling.
        if (!sHeartseedMgr->SetRank(target, uint8(rank)))
        {
            handler->PSendSysMessage("%s has no Heartseed.", target->GetName().c_str());
            return true;
        }
        handler->PSendSysMessage("Heartseed of %s is now rank %ld.", target->GetName().c_str(), rank);
        return true;
    }

    static bool HandleGrowthCommand(ChatHandler* handler, char const* args)
    {
        Player* target = GetTarget(handler);
        if (!target || !args || !*args)
            return false;

        Heartseed::State state;
        if (!sHeartseedMgr->GetState(target, state))
        {
            handler->PSendSysMessage("%s has no Heartseed.", target->GetName().c_str());
            return true;
        }

        // "+N" and "-N" change the current Growth, a bare number sets it.
        bool relative = args[0] == '+' || args[0] == '-';
        char* end = nullptr;
        long long amount = strtoll(args, &end, 10);
        if (end == args)
        {
            handler->SendSysMessage("Usage: .heartseed growth <amount> | +<amount> | -<amount>");
            handler->SetSentErrorMessage(true);
            return false;
        }

        // "+N" is Growth as if earned: it ranks the Heartseed up to the cap. A bare number or "-N" sets the raw value.
        if (relative && amount > 0)
        {
            sHeartseedMgr->AddGrowth(target, uint32(std::min(amount, (long long)UINT32_MAX)));
            Heartseed::State after;
            sHeartseedMgr->GetState(target, after);
            handler->PSendSysMessage("Heartseed of %s earned %lld Growth: rank %u -> %u (cap %u), %u Growth toward the next rank.", target->GetName().c_str(),
                amount, uint32(state.Rank), uint32(after.Rank), uint32(sHeartseedMgr->GetRankCap(target)), after.Growth);
            return true;
        }

        long long growth = relative ? (long long)state.Growth + amount : amount;
        growth = std::max(0LL, std::min(growth, (long long)UINT32_MAX));
        sHeartseedMgr->SetGrowth(target, uint32(growth));
        handler->PSendSysMessage("Heartseed of %s now has %u Growth (was %u).", target->GetName().c_str(), uint32(growth), state.Growth);
        return true;
    }

    static bool HandleFlagListCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;

        Heartseed::State state;
        if (!sHeartseedMgr->GetState(target, state))
        {
            handler->PSendSysMessage("%s has no Heartseed.", target->GetName().c_str());
            return true;
        }

        handler->PSendSysMessage("Story flags of %s: %s.", target->GetName().c_str(), DescribeFlags(state).c_str());
        handler->PSendSysMessage("Known flags: %u %s, %u %s, %u %s.",
            uint32(Heartseed::STORY_FLAG_FIRST_DUNGEON), HeartseedMgr::GetStoryFlagName(Heartseed::STORY_FLAG_FIRST_DUNGEON),
            uint32(Heartseed::STORY_FLAG_REACHED_PANDARIA), HeartseedMgr::GetStoryFlagName(Heartseed::STORY_FLAG_REACHED_PANDARIA),
            uint32(Heartseed::STORY_FLAG_FIRST_WORLD_BOSS), HeartseedMgr::GetStoryFlagName(Heartseed::STORY_FLAG_FIRST_WORLD_BOSS));
        return true;
    }

    static bool HandleFlagChange(ChatHandler* handler, char const* args, bool set)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;

        uint16 flag = ParseFlag(args);
        if (!flag)
        {
            handler->PSendSysMessage("Usage: .heartseed flag %s <name|id>; see .heartseed flag list", set ? "set" : "clear");
            handler->SetSentErrorMessage(true);
            return false;
        }

        if (!sHeartseedMgr->SetStoryFlag(target, flag, set))
        {
            handler->PSendSysMessage("%s has no Heartseed.", target->GetName().c_str());
            return true;
        }
        handler->PSendSysMessage("Story flag %u (%s) %s for %s.", uint32(flag), HeartseedMgr::GetStoryFlagName(flag),
            set ? "set" : "cleared", target->GetName().c_str());
        return true;
    }

    static bool HandleFlagSetCommand(ChatHandler* handler, char const* args)
    {
        return HandleFlagChange(handler, args, true);
    }

    static bool HandleFlagClearCommand(ChatHandler* handler, char const* args)
    {
        return HandleFlagChange(handler, args, false);
    }

    static char const* DescribeFeedResult(Heartseed::FeedResult result)
    {
        switch (result)
        {
            case Heartseed::FeedResult::Ok:          return "can be fed";
            case Heartseed::FeedResult::NoHeartseed: return "no Heartseed";
            case Heartseed::FeedResult::Disabled:    return "Heartseed.Enabled = 0";
            case Heartseed::FeedResult::NotWeapon:   return "not a weapon it eats (fishing poles and miscellaneous weapons are not)";
            case Heartseed::FeedResult::Excluded:    return "excluded (quest item, or its quality is worth 0 in Heartseed.Feed.QualityPct)";
            case Heartseed::FeedResult::Twink:       return "twink item; fed only when deleted";
            case Heartseed::FeedResult::Refundable:  return "still refundable at a vendor";
            case Heartseed::FeedResult::Equipped:    return "equipped; only weapons in the bags are fed";
        }
        return "unknown";
    }

    static std::string FormatPct(float pct)
    {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.1f%%", pct);
        return buffer;
    }

    static std::string DescribeEssenceChances(Heartseed::FeedOutcome const& outcome)
    {
        std::string text;
        for (auto const& [essenceId, chance] : outcome.EssenceChances)
        {
            Heartseed::EssenceInfo essence;
            if (!text.empty())
                text += ", ";
            text += std::to_string(essenceId) + " " + (sHeartseedMgr->GetEssence(essenceId, essence) ? essence.Name : std::string("?")) + " " +
                FormatPct(chance);
        }
        return text.empty() ? "none" : text;
    }

    static std::string DescribeEssences(Player const* target, Heartseed::State const& state)
    {
        uint8 openSlots = Heartseed::EssenceSlotsForLevel(target->GetLevel());
        std::string text;
        for (auto const& [essenceId, held] : state.Essences)
        {
            Heartseed::EssenceInfo essence;
            if (!text.empty())
                text += ", ";
            text += std::to_string(essenceId) + " " + (sHeartseedMgr->GetEssence(essenceId, essence) ? essence.Name : std::string("(unknown)")) +
                " x" + std::to_string(held.first);
            if (held.second)
                text += (held.second <= openSlots ? " in slot " : " in locked slot ") + std::to_string(held.second);
        }
        return (text.empty() ? std::string("none") : text) + " (" + std::to_string(openSlots) + " slots open)";
    }

    // An item link or entry from the arguments; 0 if neither.
    static uint32 ParseItemEntry(ChatHandler* handler, char const* args)
    {
        if (!args || !*args)
            return 0;
        std::string text = args;
        char* key = handler->extractKeyFromLink(&text[0], "Hitem");
        return key ? uint32(strtoul(key, nullptr, 10)) : 0;
    }

    // First item of this entry in the backpack or bags; equipped and banked ones are skipped.
    static Item* FindBagItem(Player* player, uint32 entry)
    {
        for (uint8 slot = INVENTORY_SLOT_ITEM_START; slot < INVENTORY_SLOT_ITEM_END; ++slot)
            if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                if (item->GetEntry() == entry)
                    return item;
        for (uint8 bag = INVENTORY_SLOT_BAG_START; bag < INVENTORY_SLOT_BAG_END; ++bag)
            if (Bag* container = player->GetBagByPos(bag))
                for (uint32 slot = 0; slot < container->GetBagSize(); ++slot)
                    if (Item* item = container->GetItemByPos(uint8(slot)))
                        if (item->GetEntry() == entry)
                            return item;
        return nullptr;
    }

    static bool HandleCapCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;

        Heartseed::State state;
        if (!sHeartseedMgr->GetState(target, state))
        {
            handler->PSendSysMessage("%s has no Heartseed.", target->GetName().c_str());
            return true;
        }

        std::string flags;
        for (uint16 flag : sHeartseedMgr->GetAwakeningFlags())
        {
            if (!flags.empty())
                flags += ", ";
            flags += std::to_string(flag) + " " + HeartseedMgr::GetStoryFlagName(flag) + (state.StoryFlags.count(flag) ? " (set)" : "");
        }

        uint8 cap = sHeartseedMgr->GetRankCap(target);
        uint32 next = sHeartseedMgr->GetGrowthForNextRank(state.Rank);
        handler->PSendSysMessage("Rank cap of %s: %u = level %u x %.2f + %u awakenings x %u. Awakening flags: %s.", target->GetName().c_str(), uint32(cap),
            uint32(target->GetLevel()), sHeartseedMgr->GetCapPerLevel(), sHeartseedMgr->GetAwakenings(state), sHeartseedMgr->GetCapPerAwakening(),
            flags.empty() ? "none" : flags.c_str());
        handler->PSendSysMessage("Account rank %u (this character uses %u), Growth %u, next rank costs %u.%s", uint32(state.Rank),
            uint32(Heartseed::EffectiveRank(state.Rank, cap)), state.Growth, next,
            state.Rank >= cap ? (state.Growth >= next ? " At the cap: Growth is banked until the cap rises." : " At the cap.") : "");
        return true;
    }

    static bool HandleFeedPreviewCommand(ChatHandler* handler, char const* args)
    {
        Player* target = GetTarget(handler);
        uint32 entry = ParseItemEntry(handler, args);
        ItemTemplate const* proto = entry ? sObjectMgr->GetItemTemplate(entry) : nullptr;
        if (!target || !proto)
        {
            handler->SendSysMessage("Usage: .heartseed feed preview <item link or entry>");
            handler->SetSentErrorMessage(true);
            return false;
        }

        // A carried copy answers for that exact item (upgrades, refund state); otherwise the template does.
        Item* item = FindBagItem(target, entry);
        uint32 itemLevel = item ? item->GetItemLevel() : proto->ItemLevel;
        Heartseed::FeedOutcome outcome;
        sHeartseedMgr->FeedPreview(proto, itemLevel, outcome);
        handler->PSendSysMessage("Feeding %u %s (item level %u, quality %u): +%u Growth. Essence chances: %s.", entry, proto->Name1.c_str(),
            itemLevel, proto->Quality, outcome.Growth, DescribeEssenceChances(outcome).c_str());
        if (sHeartseedMgr->IsTwinkItem(entry))
            handler->SendSysMessage("Twink item: never offered on replacement; fed when the player deletes it.");
        if (item)
            handler->PSendSysMessage("%s carries one. On replacement: %s. By GM command: %s.", target->GetName().c_str(),
                DescribeFeedResult(sHeartseedMgr->CanFeed(target, item, Heartseed::FeedSource::Replace)),
                DescribeFeedResult(sHeartseedMgr->CanFeed(target, item, Heartseed::FeedSource::Command)));
        else
            handler->PSendSysMessage("%s carries none in the bags.", target->GetName().c_str());
        return true;
    }

    static bool HandleFeedCommand(ChatHandler* handler, char const* args)
    {
        Player* target = GetTarget(handler);
        uint32 entry = ParseItemEntry(handler, args);
        if (!target || !entry)
        {
            handler->SendSysMessage("Usage: .heartseed feed <item link or entry> (consumes one from the bags) | .heartseed feed preview <item>");
            handler->SetSentErrorMessage(true);
            return false;
        }

        Item* item = FindBagItem(target, entry);
        if (!item)
        {
            handler->PSendSysMessage("%s carries no item %u in the bags.", target->GetName().c_str(), entry);
            handler->SetSentErrorMessage(true);
            return false;
        }

        // The same path as feeding on replacement, without the upgrade check.
        Heartseed::FeedOutcome outcome;
        Heartseed::FeedResult result = sHeartseedMgr->Feed(target, item, outcome, Heartseed::FeedSource::Command);
        if (result != Heartseed::FeedResult::Ok)
        {
            handler->PSendSysMessage("Item %u cannot be fed: %s.", entry, DescribeFeedResult(result));
            handler->SetSentErrorMessage(true);
            return false;
        }

        std::string gained;
        for (uint32 essenceId : outcome.Essences)
            gained += (gained.empty() ? "" : ", ") + std::to_string(essenceId);
        handler->PSendSysMessage("Fed item %u to %s: +%u Growth, rank %u -> %u, essences gained: %s.", entry, target->GetName().c_str(), outcome.Growth,
            uint32(outcome.OldRank), uint32(outcome.NewRank), gained.empty() ? "none" : gained.c_str());
        return true;
    }

    static bool HandleEssenceListCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = GetTarget(handler);
        Heartseed::State state;
        bool hasState = target && sHeartseedMgr->GetState(target, state);

        std::vector<Heartseed::EssenceInfo> essences = sHeartseedMgr->GetEssences();
        std::vector<Heartseed::EssenceSource> sources = sHeartseedMgr->GetEssenceSources();
        handler->PSendSysMessage("%u Heartseed essences:", uint32(essences.size()));
        for (Heartseed::EssenceInfo const& essence : essences)
        {
            std::string from;
            for (Heartseed::EssenceSource const& source : sources)
            {
                if (source.EssenceId != essence.Id)
                    continue;
                if (!from.empty())
                    from += ", ";
                from += (source.ItemEntry ? "item " + std::to_string(source.ItemEntry) : "name \"" + source.NamePattern + "\"") + " " + FormatPct(source.Chance);
            }
            handler->PSendSysMessage(" %u %s: proc spell %u, %s per hit for %.0f%% weapon damage. From %s. %s", essence.Id, essence.Name.c_str(),
                essence.ProcSpellId, FormatPct(essence.ProcChance).c_str(), essence.WeaponDamagePct, from.empty() ? "nothing" : from.c_str(),
                essence.Description.c_str());
        }
        if (hasState)
            handler->PSendSysMessage("%s holds: %s.", target->GetName().c_str(), DescribeEssences(target, state).c_str());
        return true;
    }

    static bool HandleEssenceAddCommand(ChatHandler* handler, char const* args)
    {
        Player* target = GetTarget(handler);
        char* end = nullptr;
        unsigned long essenceId = args ? strtoul(args, &end, 10) : 0;
        unsigned long count = end && *end ? strtoul(end, nullptr, 10) : 1;
        if (!target || !essenceId || !count)
        {
            handler->SendSysMessage("Usage: .heartseed essence add <essence id> [count]; see .heartseed essence list");
            handler->SetSentErrorMessage(true);
            return false;
        }
        return ReportEssence(handler, target, sHeartseedMgr->AddEssence(target, uint32(essenceId), uint32(count)), uint32(essenceId), "added");
    }

    static bool HandleEssenceRemoveCommand(ChatHandler* handler, char const* args)
    {
        Player* target = GetTarget(handler);
        unsigned long essenceId = args ? strtoul(args, nullptr, 10) : 0;
        if (!target || !essenceId)
        {
            handler->SendSysMessage("Usage: .heartseed essence remove <essence id>");
            handler->SetSentErrorMessage(true);
            return false;
        }
        return ReportEssence(handler, target, sHeartseedMgr->RemoveEssence(target, uint32(essenceId)), uint32(essenceId), "removed");
    }

    static bool HandleEssenceSlotCommand(ChatHandler* handler, char const* args)
    {
        Player* target = GetTarget(handler);
        char* end = nullptr;
        unsigned long essenceId = args ? strtoul(args, &end, 10) : 0;
        char* slotEnd = nullptr;
        long slot = end && *end ? strtol(end, &slotEnd, 10) : -1;
        if (!target || !essenceId || slotEnd == end || slot < 0 || slot > Heartseed::MaxEssenceSlots)
        {
            handler->SendSysMessage("Usage: .heartseed essence slot <essence id> <1-3, or 0 to take it out>");
            handler->SetSentErrorMessage(true);
            return false;
        }
        // GM slotting skips the level check; a slot above the level stays inactive until the level opens it.
        return ReportEssence(handler, target, sHeartseedMgr->SetEssenceSlot(target, uint32(essenceId), uint8(slot), true), uint32(essenceId),
            slot ? "slotted" : "taken out of its slot");
    }

    static bool ReportEssence(ChatHandler* handler, Player* target, Heartseed::EssenceResult result, uint32 essenceId, char const* done)
    {
        switch (result)
        {
            case Heartseed::EssenceResult::Ok:
            {
                Heartseed::State state;
                sHeartseedMgr->GetState(target, state);
                handler->PSendSysMessage("Essence %u %s for %s. Essences: %s.", essenceId, done, target->GetName().c_str(), DescribeEssences(target, state).c_str());
                return true;
            }
            case Heartseed::EssenceResult::NoHeartseed:
                handler->PSendSysMessage("%s has no Heartseed.", target->GetName().c_str());
                return true;
            case Heartseed::EssenceResult::UnknownEssence:
                handler->PSendSysMessage("Essence %u does not exist; see .heartseed essence list.", essenceId);
                break;
            case Heartseed::EssenceResult::NotCollected:
                handler->PSendSysMessage("%s has not collected essence %u.", target->GetName().c_str(), essenceId);
                break;
            case Heartseed::EssenceResult::SlotLocked:
                handler->PSendSysMessage("%s's level does not open that slot yet.", target->GetName().c_str());
                break;
            case Heartseed::EssenceResult::WrongSlot:
                handler->PSendSysMessage("Slots are 1 to %u.", uint32(Heartseed::MaxEssenceSlots));
                break;
        }
        handler->SetSentErrorMessage(true);
        return false;
    }

    static bool HandleGrantCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;

        if (!sHeartseedMgr->Grant(target))
        {
            handler->PSendSysMessage("%s already has a Heartseed.", target->GetName().c_str());
            return true;
        }
        handler->PSendSysMessage("Granted a Heartseed to %s.", target->GetName().c_str());
        return true;
    }

    // Player command: opens the Heartseed's choice window for yourself.
    static bool HandleSeedCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        if (!sHeartseedMgr->HasHeartseed(player))
        {
            handler->SendSysMessage("You do not carry a Heartseed.");
            handler->SetSentErrorMessage(true);
            return false;
        }
        if (player->IsInCombat())
        {
            handler->SendSysMessage("Your Heartseed will not listen while you fight.");
            handler->SetSentErrorMessage(true);
            return false;
        }
        sHeartseedMgr->SendChoiceMenu(player);
        return true;
    }

    // Answers the Heartseed addon (/hs) with addon messages, tab separated: "N" when the character has no
    // Heartseed, else "S" <rank> <account rank> <cap> <growth> <growth for next rank> <stage name> <dominant path>
    // <weapons fed> <open essence slots> <shrine here or empty>, then "M" <level> <trait name or empty> <path>
    // per major tier of the current spec, "X" <name> <times collected> <slot or 0> per essence, then "E".
    static bool HandleSeedSyncCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* player = handler->GetSession()->GetPlayer();
        auto send = [player](std::string message)
        {
            if (message.size() > 250)
                message.resize(250);
            WorldPacket data;
            ChatHandler::BuildChatPacket(data, CHAT_MSG_WHISPER, LANG_ADDON, player, player, message, 0, "", DEFAULT_LOCALE, HEARTSEED_ADDON_PREFIX);
            player->SendDirectMessage(&data);
        };

        Heartseed::State state;
        if (!sHeartseedMgr->GetState(player, state))
        {
            send("N");
            send("E");
            return true;
        }

        Heartseed::ShrineInfo shrine;
        std::string shrineName = sHeartseedMgr->GetShrineAt(player, shrine) ? shrine.Name : "";
        uint8 stage = sHeartseedMgr->GetStage(player);
        send("S	" + std::to_string(sHeartseedMgr->GetEffectiveRank(player)) + "	" + std::to_string(state.Rank) + "	" +
            std::to_string(sHeartseedMgr->GetRankCap(player)) + "	" + std::to_string(state.Growth) + "	" +
            std::to_string(sHeartseedMgr->GetGrowthForNextRank(state.Rank)) + "	" + sHeartseedMgr->GetStageName(stage) + "	" +
            GetPathName(sHeartseedMgr->GetDominantPath(player)) + "	" + std::to_string(state.FedWeapons) + "	" +
            std::to_string(Heartseed::EssenceSlotsForLevel(player->GetLevel())) + "	" + shrineName);

        auto loadout = state.Loadouts.find(uint16(player->GetTalentSpecialization()));
        for (uint8 tier = 0; tier < Heartseed::MaxMajorTiers; ++tier)
        {
            uint32 traitId = loadout != state.Loadouts.end() ? loadout->second[tier] : 0;
            Heartseed::TraitInfo trait;
            bool known = traitId && sHeartseedMgr->GetTrait(traitId, trait);
            send("M	" + std::to_string(Heartseed::MajorTierLevel(tier)) + "	" + (known ? trait.Name : "") + "	" +
                (known ? GetPathName(trait.TraitPath) : "") + "	" + std::to_string(known ? trait.GiftSpellId : 0) + "	" +
                (known ? trait.Description : ""));
        }

        for (auto const& [essenceId, held] : state.Essences)
        {
            Heartseed::EssenceInfo essence;
            send("X	" + (sHeartseedMgr->GetEssence(essenceId, essence) ? essence.Name : std::to_string(essenceId)) + "	" +
                std::to_string(held.first) + "	" + std::to_string(held.second));
        }
        send("E");
        return true;
    }

    static bool HandleShrinesCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;

        std::vector<Heartseed::ShrineInfo> shrines = sHeartseedMgr->GetShrines();
        handler->PSendSysMessage("%u Celestial shrines (changes %s):", uint32(shrines.size()),
            sHeartseedMgr->IsChangeAtShrineOnly() ? "only at a shrine" : "anywhere");
        for (Heartseed::ShrineInfo const& shrine : shrines)
            handler->PSendSysMessage(" %u %s: map %u (%.1f, %.1f, %.1f), radius %.0f", shrine.Id, shrine.Name.c_str(), shrine.MapId,
                shrine.X, shrine.Y, shrine.Z, shrine.Radius);
        Heartseed::ShrineInfo at;
        if (sHeartseedMgr->GetShrineAt(target, at))
            handler->PSendSysMessage("%s is at %s.", target->GetName().c_str(), at.Name.c_str());
        else
            handler->PSendSysMessage("%s is not at a shrine.", target->GetName().c_str());
        return true;
    }

    static bool HandleOfferCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;

        if (!sHeartseedMgr->OfferGrantQuest(target))
        {
            handler->PSendSysMessage("Cannot offer quest %u to %s: it already has a Heartseed, has done the quest, or the quest is missing.",
                sHeartseedMgr->GetGrantQuestId(), target->GetName().c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }
        handler->PSendSysMessage("Offered the granting quest to %s.", target->GetName().c_str());
        return true;
    }

    static bool HandleRevokeCommand(ChatHandler* handler, char const* /*args*/)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;

        if (!sHeartseedMgr->Revoke(target))
        {
            handler->PSendSysMessage("%s has no Heartseed.", target->GetName().c_str());
            return true;
        }
        handler->PSendSysMessage("Removed the Heartseed of %s, with its loadouts, essences and story flags.", target->GetName().c_str());
        return true;
    }
};

// Celestial Gifts (heartseed_gift): the active spell a level 10 major teaches. The Gift's own cooldown applies when
// it is longer than the client spell's, unless the character has the spell from its own talents (monks).
class spell_heartseed_gift : public SpellScript
{
    PrepareSpellScript(spell_heartseed_gift);

    void HandleAfterCast()
    {
        Player* player = GetCaster()->ToPlayer();
        if (!player || player->HasTalent(GetSpellInfo()->Id, player->GetActiveSpec()))
            return;
        Heartseed::TraitInfo gift;
        if (!sHeartseedMgr->GetActiveGift(player, GetSpellInfo()->Id, gift) || gift.GiftCooldown <= GetSpellInfo()->RecoveryTime)
            return;
        player->AddCustomCooldown(GetSpellInfo()->Id, Milliseconds(gift.GiftCooldown));
    }

    void Register() override
    {
        AfterCast += SpellCastFn(spell_heartseed_gift::HandleAfterCast);
    }
};

// 124947 - Celestial Breath tick. The creature spell's damage is replaced by the Gift's power, which grows with the
// main-hand item level, when Yu'lon's Gift (117960) is the source.
class spell_heartseed_celestial_breath : public SpellScript
{
    PrepareSpellScript(spell_heartseed_celestial_breath);

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;
        Heartseed::TraitInfo gift;
        if (sHeartseedMgr->GetActiveGift(player, GIFT_SPELL_CELESTIAL_BREATH, gift))
            SetHitDamage(sHeartseedMgr->GetGiftAmount(player, gift));
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_heartseed_celestial_breath::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }

    static constexpr uint32 GIFT_SPELL_CELESTIAL_BREATH = 117960;
};

void AddSC_heartseed()
{
    if (!CustomReload::Tables().RegisterGroup({ "heartseed", "heartseed_stage", "heartseed_glow", "heartseed_trait",
        "heartseed_gift", "heartseed_essence_template", "heartseed_essence_source", "twink_item", "heartseed_shrine" }, "Heartseed world data (one coherent snapshot)", []
    {
        std::size_t rows = 0;
        bool success = sHeartseedMgr->LoadData(&rows);
        return CustomReload::Result{ success, rows, success ? "Heartseed data published; characters refresh at their next trait refresh"
            : "Heartseed data could not be loaded; previous cache retained (see server log)" };
    }))
        TC_LOG_ERROR("server", "Heartseed reload loader registration failed");
    new heartseed_worldscript();
    new heartseed_playerscript();
    new heartseed_unitscript();
    new heartseed_commandscript();
    new spell_script<spell_heartseed_gift>("spell_heartseed_gift");
    new spell_script<spell_heartseed_celestial_breath>("spell_heartseed_celestial_breath");
}
