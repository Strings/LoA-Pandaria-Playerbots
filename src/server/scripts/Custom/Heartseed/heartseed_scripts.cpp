/*
 * Heartseed hooks and commands (T-020 to T-026). State lives in HeartseedMgr.
 */

#include "Bag.h"
#include "BattlegroundHooks.h"
#include "Chat.h"
#include "Custom/Reload/ReloadRegistry.h"
#include "Group.h"
#include "HeartseedMgr.h"
#include "Log.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellAuraEffects.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "SpellScript.h"
#include "TemporarySummon.h"
#include "WorldPacket.h"
#include "WorldSession.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <map>
#include <mutex>
#include <set>
#include <sstream>
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

// A Gift rider's helper aura: effect 0 carries the rider's amount, the rider's duration replaces the spell's.
static void ApplyRiderAura(Unit* caster, Unit* target, Heartseed::GiftRider const& rider, int32 amount)
{
    if (!rider.SpellId || !target->IsAlive())
        return;
    Aura* aura = caster->AddAura(rider.SpellId, target);
    if (!aura)
        return;
    if (AuraEffect* effect = aura->GetEffect(EFFECT_0))
        effect->ChangeAmount(amount);
    if (rider.Duration)
    {
        aura->SetMaxDuration(int32(rider.Duration));
        aura->SetDuration(int32(rider.Duration));
    }
}

// Set while a Gift heals, so the heal doesn't build the Hope the Gift just spent. Heals run synchronously on the
// healer's map thread.
static thread_local bool s_inGiftHeal = false;

static void GiftHeal(Unit* healer, Unit* target, SpellInfo const* spellInfo, uint32 amount)
{
    if (!amount)
        return;
    s_inGiftHeal = true;
    healer->HealBySpell(target, spellInfo, amount);
    s_inGiftHeal = false;
}

// Kill riders (Xuen's cooldown cut, Chi-Ji's reset) of the character's Gift.
static void HandleGiftKill(Player* player, Creature* killed)
{
    if (killed->GetCreatureType() == CREATURE_TYPE_CRITTER)
        return;
    Heartseed::TraitInfo gift;
    if (!sHeartseedMgr->GetActiveGift(player, 0, gift) || player->HasTalent(gift.GiftSpellId, player->GetActiveSpec()))
        return;
    for (Heartseed::GiftRider const& rider : sHeartseedMgr->GetGiftRiders(player, gift))
        if (rider.Kind == Heartseed::GiftRiderKind::KillCooldown)
            sHeartseedMgr->CutGiftCooldown(player, gift.GiftSpellId, rider.Amount);
}

// Call of Xuen: a summoned tiger can't keep up with a mount. Out of combat it runs at its owner's speed and
// jumps to them past Heartseed.Gift.XuenCatchUpDistance; checks repeat until the tiger is gone.
class HeartseedXuenFollowEvent : public BasicEvent
{
public:
    HeartseedXuenFollowEvent(Player* owner, ObjectGuid xuen) : _owner(owner), _xuen(xuen) { }

    bool Execute(uint64 /*time*/, uint32 /*diff*/) override
    {
        Creature* xuen = ObjectAccessor::GetCreature(*_owner, _xuen);
        if (!xuen || !xuen->IsAlive())
            return true;

        if (_owner->IsInCombat() || xuen->IsInCombat())
            xuen->SetSpeed(MOVE_RUN, xuen->GetCreatureTemplate()->speed_run);
        else
        {
            float ownerRate = _owner->IsFlying() ? _owner->GetSpeedRate(MOVE_FLIGHT) : _owner->GetSpeedRate(MOVE_RUN);
            xuen->SetSpeed(MOVE_RUN, std::max(ownerRate, xuen->GetCreatureTemplate()->speed_run));
            float catchUp = sHeartseedMgr->GetXuenCatchUpDistance();
            if (catchUp > 0.0f && (xuen->GetMapId() != _owner->GetMapId() || !xuen->IsWithinDist(_owner, catchUp)))
            {
                Position pos = _owner->GetNearPosition(2.0f, float(M_PI));
                if (xuen->GetMapId() == _owner->GetMapId())
                    xuen->NearTeleportTo(pos.GetPositionX(), pos.GetPositionY(), pos.GetPositionZ(), _owner->GetOrientation());
            }
        }

        _owner->m_Events.AddEventAtOffset(new HeartseedXuenFollowEvent(_owner, _xuen), sHeartseedMgr->GetXuenFollowInterval());
        return true;
    }

private:
    Player* _owner;
    ObjectGuid _xuen;
};

// Serpent's Breath cuts its cooldown once per distinct enemy a channel hits; the set restarts with each cast.
// Ticks run on map threads, so the sets are locked.
static std::mutex s_breathHitsLock;
static std::map<ObjectGuid, std::set<ObjectGuid>> s_breathHits;

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

    void OnCreatureKill(Player* player, Creature* killed) override
    {
        if (!player->GetSession()->IsBot())
            HandleGiftKill(player, killed);
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

    // Spells the player casts build Yu'lon's Coil; procs and passives don't.
    void OnSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        SpellInfo const* spellInfo = spell->GetSpellInfo();
        if (spell->IsTriggered() || spellInfo->IsPassive() || spellInfo->IsAutoRepeatRangedSpell())
            return;
        sHeartseedMgr->OnMeterEvent(player, Heartseed::MeterSource::SpellCast, 0, spellInfo->Id);
    }
};

class heartseed_unitscript : public UnitScript
{
public:
    heartseed_unitscript() : UnitScript("heartseed_unitscript") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!damage || !victim)
            return;
        // Damage taken feeds Niuzao's meter and Retort's window.
        if (victim->GetTypeId() == TYPEID_PLAYER && attacker != victim)
            sHeartseedMgr->OnMeterEvent(victim->ToPlayer(), Heartseed::MeterSource::BigHit, damage);
        if (!attacker || attacker->GetTypeId() != TYPEID_PLAYER)
            return;
        // Alts logged in as bots proc too; a character without a Heartseed returns at the first lookup.
        sHeartseedMgr->OnDamageDealt(attacker->ToPlayer(), victim);
    }

    // Healing done or received builds Chi-Ji's Hope: the health actually gained, so topping off a full target
    // builds nothing. Spell heals reach only this hook; the Gift's own heals (GiftHeal) are left out.
    void OnHeal(Unit* healer, Unit* receiver, uint32& gain) override
    {
        if (!gain || !receiver || s_inGiftHeal)
            return;
        if (healer && healer->GetTypeId() == TYPEID_PLAYER)
            sHeartseedMgr->OnMeterEvent(healer->ToPlayer(), Heartseed::MeterSource::Healing, gain);
        if (receiver->GetTypeId() == TYPEID_PLAYER && receiver != healer)
            sHeartseedMgr->OnMeterEvent(receiver->ToPlayer(), Heartseed::MeterSource::Healing, gain);
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
            { "growth", &HandleGrowthCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "points", &HandlePointsCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "flag",   flagCommandTable,     RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "feed",   feedCommandTable,     RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "essence", essenceCommandTable, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "traits", &HandleTraitsCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "major",  &HandleMajorCommand,  RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "meter",  &HandleMeterCommand,  RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "reload", &HandleReloadCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
            { "shrines", &HandleShrinesCommand, RBAC_PERM_COMMAND_HEARTSEED, Trinity::ChatCommands::Console::No },
        };
        static std::vector<ChatCommand> seedCommandTable =
        {
            { "sync", &HandleSeedSyncCommand, RBAC_PERM_COMMAND_SEED, Trinity::ChatCommands::Console::No },
            { "spend", &HandleSeedSpendCommand, RBAC_PERM_COMMAND_SEED, Trinity::ChatCommands::Console::No },
            { "reset", &HandleSeedResetCommand, RBAC_PERM_COMMAND_SEED, Trinity::ChatCommands::Console::No },
            { "choose", &HandleSeedChooseCommand, RBAC_PERM_COMMAND_SEED, Trinity::ChatCommands::Console::No },
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

        handler->PSendSysMessage("Heartseed of %s (account %u): stage %u (%s) at level %u. Growth level %u, %u of %u XP to the next, %u points unspent: %s. %u weapons fed.",
            target->GetName().c_str(), state.AccountId, uint32(stage), sHeartseedMgr->GetStageName(stage).c_str(), uint32(target->GetLevel()),
            state.GrowthLevel, state.GrowthXp, sHeartseedMgr->GetGrowthXpForLevel(state.GrowthLevel),
            Heartseed::UnspentPoints(state.GrowthLevel, state.GrowthPoints), DescribeGrowthPoints(state).c_str(), state.FedWeapons);
        handler->PSendSysMessage("Spec %u: %u of %u majors chosen, %u loadouts saved. Essences: %u collected, %u active.",
            uint32(specId), majors, uint32(Heartseed::MaxMajorTiers), uint32(state.Loadouts.size()), uint32(state.Essences.size()), activeEssences);
        Heartseed::Path dominant = sHeartseedMgr->GetDominantPath(target);
        handler->PSendSysMessage("Glow enchant %u, dominant path %u (%s). Story flags: %s. Granted %u s ago.%s",
            uint32(sHeartseedMgr->GetGlowEnchant(target)), uint32(dominant), GetPathName(dominant), DescribeFlags(state).c_str(),
            uint32(time(nullptr)) - state.GrantedTime, sHeartseedMgr->IsEnabled() ? "" : " (Heartseed.Enabled = 0)");
        handler->PSendSysMessage("Loadout: %s.", DescribeLoadout(state, specId).c_str());

        Heartseed::TraitInfo gift;
        if (sHeartseedMgr->GetActiveGift(target, 0, gift))
        {
            std::string riders;
            for (Heartseed::GiftRider const& rider : sHeartseedMgr->GetGiftRiders(target, gift))
                riders += (riders.empty() ? "" : ", ") + std::string("stage ") + std::to_string(rider.Stage) + " kind " +
                    std::to_string(uint32(rider.Kind)) + " amount " + std::to_string(rider.Amount);
            handler->PSendSysMessage("Gift spell %u, riders: %s.", gift.GiftSpellId, riders.empty() ? "none yet" : riders.c_str());
            Heartseed::MeterRule rule;
            Heartseed::MeterState meter;
            if (sHeartseedMgr->GetMeter(target, rule, meter))
            {
                Heartseed::GiftSpend spend = sHeartseedMgr->GetLastSpend(target);
                handler->PSendSysMessage("Meter: source %u, stacks %u/%u, lit %s, castable %s; last spend %u stacks, %u recent damage.",
                    uint32(rule.Source), uint32(meter.Stacks), uint32(rule.MaxStacks), meter.Lit ? "yes" : "no",
                    sHeartseedMgr->CanCastGift(target, gift.GiftSpellId) ? "yes" : "no", uint32(spend.Stacks), spend.RecentDamage);
            }
        }

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
                : "minor, level " + std::to_string(trait.MinLevel);
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

    static std::string DescribeGrowthPoints(Heartseed::State const& state)
    {
        std::string text;
        for (auto const& [statId, points] : state.GrowthPoints)
        {
            Heartseed::GrowthStat stat;
            text += (text.empty() ? "" : ", ") + (sHeartseedMgr->GetGrowthStat(statId, stat) ? stat.Name : "stat " + std::to_string(statId)) +
                " " + std::to_string(points);
        }
        return text.empty() ? "none spent" : text;
    }

    static char const* DescribeGrowthResult(Heartseed::GrowthResult result)
    {
        switch (result)
        {
            case Heartseed::GrowthResult::Ok:          return "done";
            case Heartseed::GrowthResult::NoHeartseed: return "no Heartseed";
            case Heartseed::GrowthResult::UnknownStat: return "unknown stat";
            case Heartseed::GrowthResult::NoPoints:    return "no points to move";
            case Heartseed::GrowthResult::NotAtShrine: return "only at a Celestial shrine";
        }
        return "?";
    }

    // A stat id or a name prefix ("stam", "crit"); 0 if neither.
    static uint8 ParseGrowthStat(char const* text)
    {
        if (!text || !*text)
            return 0;
        char* end = nullptr;
        long id = strtol(text, &end, 10);
        if (end != text && !*end)
            return id > 0 && id < 256 ? uint8(id) : 0;
        std::string wanted = text;
        std::transform(wanted.begin(), wanted.end(), wanted.begin(), ::tolower);
        for (Heartseed::GrowthStat const& stat : sHeartseedMgr->GetGrowthStats())
        {
            std::string name = stat.Name;
            std::transform(name.begin(), name.end(), name.begin(), ::tolower);
            if (name.compare(0, wanted.size(), wanted) == 0)
                return stat.Id;
        }
        return 0;
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

    // .heartseed meter [stacks | lit | hit <damage>]: sets the Gift's path meter (stacks, capped), lights a gated
    // Gift, or feeds it a hit taken as if in combat (Niuzao's window and big-hit check), then shows it; with no
    // argument it only shows it.
    static bool HandleMeterCommand(ChatHandler* handler, char const* args)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;
        if (args && !strncmp(args, "hit ", 4))
        {
            long damage = strtol(args + 4, nullptr, 10);
            if (damage <= 0)
            {
                handler->SendSysMessage("Usage: .heartseed meter hit <damage>");
                handler->SetSentErrorMessage(true);
                return false;
            }
            sHeartseedMgr->OnMeterEvent(target, Heartseed::MeterSource::BigHit, uint32(damage));
        }
        else if (args && *args)
        {
            bool lit = std::string(args) == "lit";
            char* end = nullptr;
            long stacks = lit ? 0 : strtol(args, &end, 10);
            if (!lit && (end == args || stacks < 0 || stacks > 255))
            {
                handler->SendSysMessage("Usage: .heartseed meter [stacks | lit]");
                handler->SetSentErrorMessage(true);
                return false;
            }
            if (!sHeartseedMgr->ForceMeter(target, uint8(stacks), lit))
            {
                handler->PSendSysMessage("%s has no Celestial Gift with a meter.", target->GetName().c_str());
                return true;
            }
        }
        Heartseed::MeterRule rule;
        Heartseed::MeterState meter;
        if (!sHeartseedMgr->GetMeter(target, rule, meter))
        {
            handler->PSendSysMessage("%s has no Celestial Gift with a meter.", target->GetName().c_str());
            return true;
        }
        handler->PSendSysMessage("Meter of %s: stacks %u/%u, lit %s.", target->GetName().c_str(), uint32(meter.Stacks),
            uint32(rule.MaxStacks), meter.Lit ? "yes" : "no");
        return true;
    }

    // .heartseed growth: show; "+N" earns N Growth XP as in play (bots and the twink rule still apply); "<level>
    // [xp]" sets the Growth level and XP directly.
    static bool HandleGrowthCommand(ChatHandler* handler, char const* args)
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

        if (args && *args)
        {
            char* end = nullptr;
            bool earn = args[0] == '+';
            long long first = strtoll(earn ? args + 1 : args, &end, 10);
            long long second = 0;
            if (end != (earn ? args + 1 : args) && !earn && *end)
                second = strtoll(end, nullptr, 10);
            if (end == (earn ? args + 1 : args) || first < 0 || second < 0)
            {
                handler->SendSysMessage("Usage: .heartseed growth [+<xp> | <level> [xp]]");
                handler->SetSentErrorMessage(true);
                return false;
            }
            if (earn)
                sHeartseedMgr->AddGrowthXp(target, uint32(std::min(first, (long long)UINT32_MAX)));
            else
                sHeartseedMgr->SetGrowthLevel(target, uint32(std::min(first, (long long)UINT32_MAX)), uint32(std::min(second, (long long)UINT32_MAX)));
            sHeartseedMgr->GetState(target, state);
        }

        handler->PSendSysMessage("Growth of %s: level %u, %u of %u XP to the next, %u points unspent (%s).", target->GetName().c_str(),
            state.GrowthLevel, state.GrowthXp, sHeartseedMgr->GetGrowthXpForLevel(state.GrowthLevel),
            Heartseed::UnspentPoints(state.GrowthLevel, state.GrowthPoints), DescribeGrowthPoints(state).c_str());
        return true;
    }

    // .heartseed points: list the stats; "spend <stat> [n]" spends for the target; "reset" refunds anywhere.
    static bool HandlePointsCommand(ChatHandler* handler, char const* args)
    {
        Player* target = GetTarget(handler);
        if (!target)
            return false;

        std::string text = args ? args : "";
        std::istringstream in(text);
        std::string verb, statText;
        uint32 count = 1;
        in >> verb >> statText >> count;

        if (verb == "spend")
            handler->PSendSysMessage("Spend on %s: %s.", statText.c_str(),
                DescribeGrowthResult(sHeartseedMgr->SpendGrowthPoints(target, ParseGrowthStat(statText.c_str()), count)));
        else if (verb == "reset")
            handler->PSendSysMessage("Reset: %s.", DescribeGrowthResult(sHeartseedMgr->ResetGrowthPoints(target, true)));
        else if (!verb.empty())
        {
            handler->SendSysMessage("Usage: .heartseed points [spend <stat id or name> [count] | reset]");
            handler->SetSentErrorMessage(true);
            return false;
        }

        Heartseed::State state;
        sHeartseedMgr->GetState(target, state);
        for (Heartseed::GrowthStat const& stat : sHeartseedMgr->GetGrowthStats())
        {
            auto spent = state.GrowthPoints.find(stat.Id);
            uint32 points = spent != state.GrowthPoints.end() ? spent->second : 0;
            handler->PSendSysMessage(" %u %s: %u points, +%d%% (spell %u)", uint32(stat.Id), stat.Name.c_str(), points,
                Heartseed::StatPointAmount(points, stat.MaxPct, stat.HalfPoints), HeartseedMgr::GetGrowthStatSpell(target, stat));
        }
        handler->PSendSysMessage("%s: Growth level %u, %u points unspent.", target->GetName().c_str(), state.GrowthLevel,
            Heartseed::UnspentPoints(state.GrowthLevel, state.GrowthPoints));
        return true;
    }

    // Player: .seed spend <stat> [count], the Heartseed addon's stat buttons.
    static bool HandleSeedSpendCommand(ChatHandler* handler, char const* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        std::istringstream in(args ? args : "");
        std::string statText;
        uint32 count = 1;
        in >> statText >> count;
        uint8 statId = ParseGrowthStat(statText.c_str());
        Heartseed::GrowthStat stat;
        if (!sHeartseedMgr->GetGrowthStat(statId, stat))
        {
            std::string names;
            for (Heartseed::GrowthStat const& known : sHeartseedMgr->GetGrowthStats())
                names += (names.empty() ? "" : ", ") + known.Name;
            handler->PSendSysMessage("Usage: .seed spend <stat> [count]. Stats: %s.", names.c_str());
            handler->SetSentErrorMessage(true);
            return false;
        }

        Heartseed::GrowthResult result = sHeartseedMgr->SpendGrowthPoints(player, statId, count);
        if (result == Heartseed::GrowthResult::Ok)
            handler->PSendSysMessage("Your Heartseed strengthens your %s.", stat.Name.c_str());
        else
            handler->PSendSysMessage("Your Heartseed has %s.", result == Heartseed::GrowthResult::NoPoints ? "no stat points to spend" : DescribeGrowthResult(result));
        return true;
    }

    // Player: .seed choose <Celestial> [level], the Heartseed addon's Celestial buttons: takes that Celestial's major
    // at the tier opening at <level> (default the first, the Gift). Same rules as the choice window (.seed).
    static bool HandleSeedChooseCommand(ChatHandler* handler, char const* args)
    {
        Player* player = handler->GetSession()->GetPlayer();
        std::istringstream in(args ? args : "");
        std::string pathText;
        uint32 level = Heartseed::MajorTierLevel(0);
        in >> pathText >> level;

        // Letters only, any case: "Yu'lon", "yulon" and "Chi-Ji", "chiji" all match.
        auto letters = [](std::string const& text)
        {
            std::string out;
            for (char c : text)
                if (std::isalpha(static_cast<unsigned char>(c)))
                    out += char(std::tolower(static_cast<unsigned char>(c)));
            return out;
        };
        Heartseed::Path path = Heartseed::Path::None;
        for (Heartseed::Path candidate : { Heartseed::Path::Xuen, Heartseed::Path::ChiJi, Heartseed::Path::Niuzao, Heartseed::Path::Yulon })
            if (!pathText.empty() && letters(pathText) == letters(GetPathName(candidate)))
                path = candidate;
        uint8 tier = Heartseed::MaxMajorTiers;
        for (uint8 t = 0; t < Heartseed::MaxMajorTiers; ++t)
            if (Heartseed::MajorTierLevel(t) == level)
                tier = t;
        if (path == Heartseed::Path::None || tier >= Heartseed::MaxMajorTiers)
        {
            handler->SendSysMessage("Usage: .seed choose <Xuen|Niuzao|Yu'lon|Chi-Ji> [level of the major, default the Gift's].");
            handler->SetSentErrorMessage(true);
            return false;
        }

        Heartseed::State state;
        if (!sHeartseedMgr->GetState(player, state))
        {
            handler->SendSysMessage("You do not carry a Heartseed.");
            return true;
        }
        if (player->IsInCombat())
        {
            handler->SendSysMessage("Your Heartseed will not listen while you fight.");
            return true;
        }
        if (!Heartseed::MajorTierOpen(tier, player->GetLevel()))
        {
            handler->PSendSysMessage("Your Heartseed's level %u path opens at level %u.", level, level);
            return true;
        }

        Heartseed::TraitInfo pick;
        bool found = false;
        for (Heartseed::TraitInfo const& trait : sHeartseedMgr->GetTraits())
        {
            if (trait.Kind == Heartseed::TraitKind::Major && trait.Tier == tier && trait.TraitPath == path)
            {
                pick = trait;
                found = true;
                break;
            }
        }
        if (!found)
        {
            handler->PSendSysMessage("%s is silent at level %u for now.", GetPathName(path), level);
            return true;
        }

        auto loadout = state.Loadouts.find(uint16(player->GetTalentSpecialization()));
        uint32 chosen = loadout != state.Loadouts.end() ? loadout->second[tier] : 0;
        if (chosen == pick.Id)
        {
            handler->PSendSysMessage("Your Heartseed already walks with %s: %s.", GetPathName(path), pick.Name.c_str());
            return true;
        }
        Heartseed::ShrineInfo shrine;
        if (chosen && sHeartseedMgr->IsChangeAtShrineOnly() && !sHeartseedMgr->GetShrineAt(player, shrine))
        {
            handler->SendSysMessage("Your Heartseed holds to the path it took. Visit a Celestial shrine to change it.");
            return true;
        }
        if (sHeartseedMgr->ChooseMajor(player, tier, pick.Id, false) == Heartseed::ChoiceResult::Ok)
            handler->PSendSysMessage("Your Heartseed turns toward %s: %s.", GetPathName(path), pick.Name.c_str());
        return true;
    }

    // Player: .seed reset takes every point back, at a Celestial shrine (Heartseed.Growth.ResetAtShrineOnly).
    static bool HandleSeedResetCommand(ChatHandler* handler, char const* /*args*/)
    {
        Heartseed::GrowthResult result = sHeartseedMgr->ResetGrowthPoints(handler->GetSession()->GetPlayer(), false);
        if (result == Heartseed::GrowthResult::Ok)
            handler->SendSysMessage("Your Heartseed lets its strength go. Spend the points again as you like.");
        else if (result == Heartseed::GrowthResult::NotAtShrine)
            handler->SendSysMessage("Only a Celestial shrine can take your Heartseed's points back.");
        else
            handler->PSendSysMessage("Nothing to take back: %s.", DescribeGrowthResult(result));
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
        handler->PSendSysMessage("Feeding %u %s (item level %u, quality %u): essence chances: %s.", entry, proto->Name1.c_str(),
            itemLevel, proto->Quality, DescribeEssenceChances(outcome).c_str());
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
        handler->PSendSysMessage("Fed item %u to %s, essences gained: %s.", entry, target->GetName().c_str(), gained.empty() ? "none" : gained.c_str());
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
    // Heartseed, else "S" <Growth level> <unspent points> <Growth XP> <XP for next level> <stage name> <dominant path>
    // <weapons fed> <open essence slots> <shrine here or empty>, then "M" <level> <trait name or empty> <path>
    // per major tier of the current spec, "X" <name> <times collected> <slot or 0> per essence, "G" <stat id> <name>
    // <points> <% per point> per Growth stat, then "E".
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
        send("S	" + std::to_string(state.GrowthLevel) + "	" + std::to_string(Heartseed::UnspentPoints(state.GrowthLevel, state.GrowthPoints)) + "	" +
            std::to_string(state.GrowthXp) + "	" + std::to_string(sHeartseedMgr->GetGrowthXpForLevel(state.GrowthLevel)) + "	" +
            sHeartseedMgr->GetStageName(stage) + "	" +
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
        for (Heartseed::GrowthStat const& stat : sHeartseedMgr->GetGrowthStats())
        {
            auto spent = state.GrowthPoints.find(stat.Id);
            uint32 points = spent != state.GrowthPoints.end() ? spent->second : 0;
            int32 now = Heartseed::StatPointAmount(points, stat.MaxPct, stat.HalfPoints);
            send("G	" + std::to_string(stat.Id) + "	" + stat.Name + "	" + std::to_string(points) + "	" + std::to_string(now) +
                "	" + std::to_string(Heartseed::StatPointAmount(points + 1, stat.MaxPct, stat.HalfPoints) - now) + "	" +
                std::to_string(stat.MaxPct));
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

// Celestial Gifts (heartseed_gift): the active spell a level 10 major teaches. The Gift's cooldown and summon
// lifetime replace the client spell's, unless the character has the spell from its own talents (monks). Casting it
// spends the path meter (heartseed_meter); a gated Gift (Niuzao's Retort) casts only while its meter is lit.
class spell_heartseed_gift : public SpellScript
{
    PrepareSpellScript(spell_heartseed_gift);

    // The character's Gift cast by the character, not a proc of the same spell (Spirit of Chi-Ji from the cloak).
    Player* GiftCaster()
    {
        Player* player = GetCaster()->ToPlayer();
        if (!player || GetSpell()->IsTriggered() || player->HasTalent(GetSpellInfo()->Id, player->GetActiveSpec()))
            return nullptr;
        return player;
    }

    SpellCastResult CheckMeter()
    {
        Player* player = GiftCaster();
        if (player && !sHeartseedMgr->CanCastGift(player, GetSpellInfo()->Id))
            return SPELL_FAILED_CASTER_AURASTATE;
        return SPELL_CAST_OK;
    }

    // Before the Gift's effects land, so they read what it spent.
    void HandleOnCast()
    {
        if (Player* player = GiftCaster())
            sHeartseedMgr->SpendGiftMeter(player, GetSpellInfo()->Id);
    }

    void HandleAfterCast()
    {
        Player* player = GiftCaster();
        if (!player)
            return;
        uint32 spellId = GetSpellInfo()->Id;
        Heartseed::TraitInfo gift;
        if (!sHeartseedMgr->GetActiveGift(player, spellId, gift))
            return;
        Heartseed::GiftSpend spend = sHeartseedMgr->GetLastSpend(player);

        if (gift.GiftCooldown)
        {
            // Clearing first lets the Gift be shorter than the spell's own cooldown (Invoke Xuen's 3 minutes).
            player->RemoveSpellCooldown(spellId, true);
            player->AddCustomCooldown(spellId, Milliseconds(gift.GiftCooldown));
        }

        if (GetSpellInfo()->Effects[EFFECT_0].Effect == SPELL_EFFECT_SUMMON)
        {
            uint32 entry = uint32(GetSpellInfo()->Effects[EFFECT_0].MiscValue);
            // Xuen: each Thunder spent keeps the tiger longer.
            uint32 duration = Heartseed::SpendScaled(gift.GiftDuration, spend.Stacks, spend.BonusPctPerStack);
            for (TempSummon* summon : player->GetSummons())
            {
                if (summon->GetEntry() != entry)
                    continue;
                if (duration && summon->GetTimer() != duration)
                    summon->UnSummon(duration);
                player->m_Events.AddEventAtOffset(new HeartseedXuenFollowEvent(player, summon->GetGUID()), sHeartseedMgr->GetXuenFollowInterval());
            }
        }

        {
            std::lock_guard<std::mutex> guard(s_breathHitsLock);
            s_breathHits.erase(player->GetGUID());
        }

        for (Heartseed::GiftRider const& rider : sHeartseedMgr->GetGiftRiders(player, gift))
        {
            switch (rider.Kind)
            {
                case Heartseed::GiftRiderKind::SelfAura:
                    ApplyRiderAura(player, player, rider, rider.Amount);
                    break;
                case Heartseed::GiftRiderKind::HealPct:
                    GiftHeal(player, player, GetSpellInfo(), Heartseed::PercentOf(player->GetMaxHealth(), rider.Amount));
                    break;
                case Heartseed::GiftRiderKind::RecentDamageShield:
                    if (uint32 shield = Heartseed::PercentOf(spend.RecentDamage, rider.Amount))
                        ApplyRiderAura(player, player, rider, int32(shield));
                    break;
                default:
                    break;
            }
        }
    }

    void Register() override
    {
        OnCheckCast += SpellCheckCastFn(spell_heartseed_gift::CheckMeter);
        OnCast += SpellCastFn(spell_heartseed_gift::HandleOnCast);
        AfterCast += SpellCastFn(spell_heartseed_gift::HandleAfterCast);
    }
};

// 310009 - Heartseed meter passive. spell_proc_event makes it proc on critical hits and heals the character
// deals; a crit builds Xuen's Thunder and cuts the Gift cooldown on paths that say so.
class spell_heartseed_meter : public AuraScript
{
    PrepareAuraScript(spell_heartseed_meter);

    void HandleProc(ProcEventInfo& eventInfo)
    {
        Player* player = GetTarget()->ToPlayer();
        if (!player || !(eventInfo.GetHitMask() & PROC_EX_CRITICAL_HIT))
            return;
        uint32 spellId = eventInfo.GetSpellInfo() ? eventInfo.GetSpellInfo()->Id : 0;
        sHeartseedMgr->OnMeterEvent(player, Heartseed::MeterSource::Crit, 0, spellId);
    }

    void Register() override
    {
        OnProc += AuraProcFn(spell_heartseed_meter::HandleProc);
    }
};

// 310101 - Crane's Plume, Chi-Ji's Gift (a clone of Spirit of Chi-Ji): heals the caster and the most wounded group members
// nearby for a share of the caster's health, more for each Hope spent. The cloak's own proc is left alone.
class spell_heartseed_crane_plume : public SpellScript
{
    PrepareSpellScript(spell_heartseed_crane_plume);

    bool Load() override
    {
        Player* player = GetCaster()->ToPlayer();
        if (!player || GetSpell()->IsTriggered() || !sHeartseedMgr->GetActiveGift(player, GetSpellInfo()->Id, _gift))
            return true;
        _active = true;
        for (Heartseed::GiftRider const& rider : sHeartseedMgr->GetGiftRiders(player, _gift))
            if (rider.Kind == Heartseed::GiftRiderKind::ExtraTargets)
                _extraTargets += std::max(0, rider.Amount);
        return true;
    }

    void SelectAllies(std::list<WorldObject*>& targets)
    {
        if (!_active)
            return;
        Player* caster = GetCaster()->ToPlayer();
        float radius = sHeartseedMgr->GetPlumeRadius();
        std::vector<Unit*> allies;
        if (Group* group = caster->GetGroup())
        {
            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
            {
                Player* member = ref->GetSource();
                if (member && member != caster && member->IsAlive() && member->IsInMap(caster) && caster->IsWithinDist(member, radius)
                    && caster->IsWithinLOSInMap(member))
                    allies.push_back(member);
            }
        }
        // The most wounded first.
        std::sort(allies.begin(), allies.end(), [](Unit* a, Unit* b) { return a->GetHealthPct() < b->GetHealthPct(); });
        uint32 count = sHeartseedMgr->GetPlumeTargets() + _extraTargets;
        if (allies.size() > count)
            allies.resize(count);

        targets.clear();
        targets.push_back(caster);
        for (Unit* ally : allies)
            targets.push_back(ally);
    }

    // The heal goes through GiftHeal instead of the spell's own, so it builds no Hope.
    void HandleHeal(SpellEffIndex effIndex)
    {
        if (!_active || !GetHitUnit())
            return;
        PreventHitDefaultEffect(effIndex);
        Player* caster = GetCaster()->ToPlayer();
        Heartseed::GiftSpend spend = sHeartseedMgr->GetLastSpend(caster);
        GiftHeal(caster, GetHitUnit(), GetSpellInfo(), Heartseed::SpendScaled(Heartseed::PercentOf(caster->GetMaxHealth(),
            _gift.GiftBaseAmount), spend.Stacks, spend.BonusPctPerStack));
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_heartseed_crane_plume::SelectAllies, EFFECT_0, TARGET_UNIT_DEST_AREA_ALLY);
        OnEffectHitTarget += SpellEffectFn(spell_heartseed_crane_plume::HandleHeal, EFFECT_0, SPELL_EFFECT_HEAL);
    }

    Heartseed::TraitInfo _gift;
    bool _active = false;
    uint32 _extraTargets = 0;
};

// 310102 - Niuzao's Retort, Niuzao's Gift (a clone of Ox Charge), castable only for a short time after a big hit (the gift
// script gates it): a shockwave around the caster deals a share of the damage taken in the meter window to each
// enemy. Riders weaken the enemies hit, widen the shockwave and cut the cooldown per enemy hit; the shield is the
// gift script's.
class spell_heartseed_niuzao_retort : public SpellScript
{
    PrepareSpellScript(spell_heartseed_niuzao_retort);

    bool Load() override
    {
        Player* player = GetCaster()->ToPlayer();
        if (!player || GetSpell()->IsTriggered() || !sHeartseedMgr->GetActiveGift(player, GetSpellInfo()->Id, _gift))
            return true;
        _active = true;
        _riders = sHeartseedMgr->GetGiftRiders(player, _gift);
        return true;
    }

    Heartseed::GiftRider const* Rider(Heartseed::GiftRiderKind kind) const
    {
        for (Heartseed::GiftRider const& rider : _riders)
            if (rider.Kind == kind)
                return &rider;
        return nullptr;
    }

    void SelectEnemies(std::list<WorldObject*>& targets)
    {
        if (!_active)
            return;
        Unit* caster = GetCaster();
        float radius = sHeartseedMgr->GetRetortRadius();
        if (Heartseed::GiftRider const* rider = Rider(Heartseed::GiftRiderKind::Widen))
            radius *= (100.0f + float(std::max(0, rider->Amount))) / 100.0f;
        std::list<Unit*> nearby;
        caster->GetAttackableUnitListInRange(nearby, radius);
        targets.clear();
        for (Unit* unit : nearby)
            if (unit != caster && caster->IsValidAttackTarget(unit) && caster->IsWithinLOSInMap(unit))
                targets.push_back(unit);
    }

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        if (!_active)
            return;
        Player* caster = GetCaster()->ToPlayer();
        SetHitDamage(int32(Heartseed::PercentOf(sHeartseedMgr->GetLastSpend(caster).RecentDamage, _gift.GiftBaseAmount)));
        if (Unit* target = GetHitUnit())
            if (Heartseed::GiftRider const* rider = Rider(Heartseed::GiftRiderKind::TargetAura))
                ApplyRiderAura(caster, target, *rider, rider->Amount);
        if (Heartseed::GiftRider const* rider = Rider(Heartseed::GiftRiderKind::HitCooldown))
            sHeartseedMgr->CutGiftCooldown(caster, _gift.GiftSpellId, rider->Amount);
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_heartseed_niuzao_retort::SelectEnemies, EFFECT_0, TARGET_UNIT_DEST_AREA_ENEMY);
        OnEffectHitTarget += SpellEffectFn(spell_heartseed_niuzao_retort::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }

    Heartseed::TraitInfo _gift;
    std::vector<Heartseed::GiftRider> _riders;
    bool _active = false;
};

// 124947 - Celestial Breath tick. The creature spell's damage is replaced by the Gift's power, a share of the caster's
// attack or spell power raised by the Coil spent, when Yu'lon's Gift (310103) is the source.
class spell_heartseed_celestial_breath : public SpellScript
{
    PrepareSpellScript(spell_heartseed_celestial_breath);

    // The creature spell reaches 100 yards, Yu'lon's size; a player's breath reaches Heartseed.Gift.BreathRange.
    void FilterTargets(std::list<WorldObject*>& targets)
    {
        Unit* caster = GetCaster();
        if (!caster || caster->GetTypeId() != TYPEID_PLAYER)
            return;
        float range = sHeartseedMgr->GetBreathRange();
        targets.remove_if([caster, range](WorldObject* target) { return !caster->IsWithinDist(target, range); });
    }

    void HandleDamage(SpellEffIndex /*effIndex*/)
    {
        Player* player = GetCaster() ? GetCaster()->ToPlayer() : nullptr;
        if (!player)
            return;
        Heartseed::TraitInfo gift;
        if (!sHeartseedMgr->GetActiveGift(player, GIFT_SPELL_CELESTIAL_BREATH, gift))
            return;
        Heartseed::GiftSpend spend = sHeartseedMgr->GetLastSpend(player);
        int32 power = int32(Heartseed::SpendScaled(uint32(std::max(sHeartseedMgr->GetGiftAmount(player, gift), 0)), spend.Stacks,
            spend.BonusPctPerStack));
        SetHitDamage(power);

        Unit* target = GetHitUnit();
        for (Heartseed::GiftRider const& rider : sHeartseedMgr->GetGiftRiders(player, gift))
        {
            switch (rider.Kind)
            {
                case Heartseed::GiftRiderKind::LeechPct:
                    GiftHeal(player, player, GetSpellInfo(), Heartseed::PercentOf(uint32(std::max(power, 0)), rider.Amount));
                    break;
                case Heartseed::GiftRiderKind::TargetDot:
                    if (target)
                        ApplyRiderAura(player, target, rider, int32(Heartseed::PercentOf(uint32(std::max(power, 0)), rider.Amount)));
                    break;
                case Heartseed::GiftRiderKind::HitCooldown:
                {
                    bool firstHit = false;
                    if (target)
                    {
                        std::lock_guard<std::mutex> guard(s_breathHitsLock);
                        firstHit = s_breathHits[player->GetGUID()].insert(target->GetGUID()).second;
                    }
                    if (firstHit)
                        sHeartseedMgr->CutGiftCooldown(player, GIFT_SPELL_CELESTIAL_BREATH, rider.Amount);
                    break;
                }
                default:
                    break;
            }
        }
    }

    void Register() override
    {
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_heartseed_celestial_breath::FilterTargets, EFFECT_0,
            TARGET_UNIT_CONE_ENEMY_104);
        OnEffectHitTarget += SpellEffectFn(spell_heartseed_celestial_breath::HandleDamage, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
    }

    static constexpr uint32 GIFT_SPELL_CELESTIAL_BREATH = 310103; // Serpent's Breath
};

// 123996 - Crackling Tiger Lightning. The creature spell has no visible bolt, so Xuen's strikes play a lightning
// visual (Heartseed.Gift.XuenLightningVisual) from the tiger to each target. Applies to monks' Xuen too.
class spell_heartseed_xuen_lightning : public SpellScript
{
    PrepareSpellScript(spell_heartseed_xuen_lightning);

    void HandleHit(SpellEffIndex /*effIndex*/)
    {
        uint32 visual = sHeartseedMgr->GetXuenLightningVisual();
        if (visual && GetCaster() && GetHitUnit())
            GetCaster()->SendPlaySpellVisual(visual, GetHitUnit()->GetGUID(), 0.0f);
    }

    // Bloom rider: the lightning arcs to more enemies near the ones it already strikes.
    void AddChainTargets(std::list<WorldObject*>& targets)
    {
        Unit* xuen = GetCaster();
        Player* owner = xuen->GetOwner() ? xuen->GetOwner()->ToPlayer() : nullptr;
        Heartseed::GiftRider rider;
        if (!owner || targets.empty() || owner->HasTalent(MONK_TALENT_INVOKE_XUEN, owner->GetActiveSpec())
            || !sHeartseedMgr->GetGiftRider(owner, GIFT_SPELL_CALL_OF_XUEN, Heartseed::GiftRiderKind::ExtraTargets, rider))
            return;

        std::list<Unit*> nearby;
        xuen->GetAttackableUnitListInRange(nearby, CHAIN_RANGE);
        int32 added = 0;
        for (Unit* unit : nearby)
        {
            if (added >= rider.Amount)
                break;
            if (std::find(targets.begin(), targets.end(), unit) != targets.end() || !xuen->IsValidAttackTarget(unit)
                || !xuen->IsWithinLOSInMap(unit))
                continue;
            targets.push_back(unit);
            ++added;
        }
    }

    void Register() override
    {
        OnEffectHitTarget += SpellEffectFn(spell_heartseed_xuen_lightning::HandleHit, EFFECT_0, SPELL_EFFECT_SCHOOL_DAMAGE);
        OnObjectAreaTargetSelect += SpellObjectAreaTargetSelectFn(spell_heartseed_xuen_lightning::AddChainTargets, EFFECT_0, TARGET_UNIT_SRC_AREA_ENEMY);
    }

    static constexpr uint32 GIFT_SPELL_CALL_OF_XUEN = 310100;
    static constexpr uint32 MONK_TALENT_INVOKE_XUEN = 123904;
    static constexpr float CHAIN_RANGE = 15.0f;
};

void AddSC_heartseed()
{
    if (!CustomReload::Tables().RegisterGroup({ "heartseed", "heartseed_stage", "heartseed_glow", "heartseed_trait",
        "heartseed_gift", "heartseed_gift_rider", "heartseed_meter", "heartseed_essence_template", "heartseed_essence_source", "twink_item", "heartseed_shrine", "heartseed_growth_stat" }, "Heartseed world data (one coherent snapshot)", []
    {
        std::size_t rows = 0;
        bool success = sHeartseedMgr->LoadData(&rows);
        return CustomReload::Result{ success, rows, success ? "Heartseed data published; characters refresh at their next trait refresh"
            : "Heartseed data could not be loaded; previous cache retained (see server log)" };
    }))
        TC_LOG_ERROR("server", "Heartseed reload loader registration failed");
    BattlegroundHooks::Register([](Player* player, bool won)
    {
        if (!player->GetSession()->IsBot())
            sHeartseedMgr->OnBattlegroundEnd(player, won);
    });
    new heartseed_worldscript();
    new heartseed_playerscript();
    new heartseed_unitscript();
    new heartseed_commandscript();
    new spell_script<spell_heartseed_gift>("spell_heartseed_gift");
    new spell_script<spell_heartseed_celestial_breath>("spell_heartseed_celestial_breath");
    new spell_script<spell_heartseed_xuen_lightning>("spell_heartseed_xuen_lightning");
    new aura_script<spell_heartseed_meter>("spell_heartseed_meter");
    new spell_script<spell_heartseed_crane_plume>("spell_heartseed_crane_plume");
    new spell_script<spell_heartseed_niuzao_retort>("spell_heartseed_niuzao_retort");
}
