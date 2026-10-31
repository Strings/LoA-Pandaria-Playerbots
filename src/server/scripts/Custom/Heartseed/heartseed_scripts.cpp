/*
 * Heartseed hooks and GM commands (T-020, T-021, T-024). State lives in HeartseedMgr.
 */

#include "Chat.h"
#include "HeartseedMgr.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "WorldSession.h"

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <string>

// Heartseed block of docs/ID_RANGES.md; linked to role 197 by sql/updates/master/auth/2026_10_31_claude_02_heartseed_command.sql
static rbac::RBACPermissions const RBAC_PERM_COMMAND_HEARTSEED = static_cast<rbac::RBACPermissions>(90000);

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
        sHeartseedMgr->LoadData();
    }
};

class heartseed_playerscript : public PlayerScript
{
public:
    heartseed_playerscript() : PlayerScript("heartseed_playerscript") { }

    void OnLogin(Player* player) override
    {
        // Bots never get a Heartseed; skipping them saves four queries per bot login.
        if (player->GetSession()->IsBot())
            return;

        sHeartseedMgr->LoadPlayer(player);
        // The inventory was equipped before the state loaded, so apply traits and send the glow now.
        sHeartseedMgr->Refresh(player);
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

        // The shrine gossip (T-023) is where the choice is made; this tells the player a choice is waiting.
        uint16 specId = uint16(player->GetTalentSpecialization());
        auto loadout = state.Loadouts.find(specId);
        for (uint8 tier = 0; tier < Heartseed::MaxMajorTiers; ++tier)
        {
            uint8 tierLevel = Heartseed::MajorTierLevels[tier];
            bool chosen = loadout != state.Loadouts.end() && loadout->second[tier];
            if (oldLevel < tierLevel && player->GetLevel() >= tierLevel && !chosen)
            {
                ChatHandler(player->GetSession()).SendSysMessage("Your Heartseed stirs. A Celestial path waits for your choice.");
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

    void OnVisibleItemChanged(Player* player, uint8 slot, Item* /*item*/) override
    {
        // Traits scale with the main-hand weapon. During login the inventory loads before the state; OnLogin
        // applies the traits then.
        if (slot == EQUIPMENT_SLOT_MAINHAND && player->IsInWorld() && !player->GetSession()->IsBot())
            sHeartseedMgr->RefreshTraits(player);
    }

    void OnSpecializationChanged(Player* player) override
    {
        // Each specialization has its own loadout, and the glow follows its dominant path.
        if (!player->GetSession()->IsBot())
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
        Player* player = attacker->ToPlayer();
        if (!player->GetSession()->IsBot())
            sHeartseedMgr->OnDamageDealt(player, victim);
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
        static std::vector<ChatCommand> heartseedCommandTable =
        {
            { "info",   &HandleInfoCommand,   RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "grant",  &HandleGrantCommand,  RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "revoke", &HandleRevokeCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "rank",   &HandleRankCommand,   RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "growth", &HandleGrowthCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "flag",   flagCommandTable,     RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "traits", &HandleTraitsCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "major",  &HandleMajorCommand,  RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "reload", &HandleReloadCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "heartseed", heartseedCommandTable, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
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
        for (auto const& essence : state.Essences)
            activeEssences += essence.second.second ? 1 : 0;

        handler->PSendSysMessage("Heartseed of %s: rank %u, Growth %u, stage %u (%s) at level %u.",
            target->GetName().c_str(), uint32(state.Rank), state.Growth, uint32(stage), sHeartseedMgr->GetStageName(stage).c_str(), uint32(target->GetLevel()));
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
            text += "level " + std::to_string(Heartseed::MajorTierLevels[tier]) + " " + std::to_string(traitId) + " " +
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
                ? "major, level " + std::to_string(Heartseed::MajorTierLevels[trait.Tier])
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
        // Stages, glows and traits. New spells in spell_dbc still need a restart.
        sHeartseedMgr->LoadData();
        Player* target = GetTarget(handler);
        if (target && sHeartseedMgr->HasHeartseed(target))
            sHeartseedMgr->Refresh(target);
        handler->SendSysMessage("Heartseed stages, glows and traits reloaded. Other characters pick up trait changes at their next refresh (relog, level, weapon or spec change).");
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

void AddSC_heartseed()
{
    new heartseed_worldscript();
    new heartseed_playerscript();
    new heartseed_unitscript();
    new heartseed_commandscript();
}
