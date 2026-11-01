#include "QoLPolicy.h"
#include "Chat.h"
#include "Config.h"
#include "CustomTransmogrification.h"
#include "DBCStores.h"
#include "Item.h"
#include "ObjectAccessor.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "PassiveAI.h"
#include "RBAC.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "SpellHistory.h"
#include "Spell.h"
#include "SpellScript.h"
#include "World.h"
#include "WorldSession.h"
#include <mutex>
#include <sstream>
#include <vector>

namespace
{
    constexpr uint32 HearthstoneSpell = 8690;
    constexpr uint32 HearthstoneItem = 6948;
    constexpr uint32 ServicesText = 94000;
    rbac::RBACPermissions const Permission = static_cast<rbac::RBACPermissions>(94000);
    std::mutex pendingMutex;
    std::vector<ObjectGuid> pending;

    bool AllPerks() { return sConfigMgr->GetBoolDefault("SoloQoL.GuildPerksForAll", true); }

    void ReconcilePerks(Player* player)
    {
        bool all = AllPerks();
        for (uint32 i = 0; i < sGuildPerkSpellsStore.GetNumRows(); ++i)
            if (GuildPerkSpellsEntry const* perk = sGuildPerkSpellsStore.LookupEntry(i))
            {
                bool wanted = QoLPolicy::WantPerk(all, player->GetGuildId() != 0, player->GetGuildLevel(), perk->Level);
                if (wanted && !player->HasSpell(perk->SpellId))
                    player->LearnSpell(perk->SpellId, false);
                else if (!wanted && player->HasSpell(perk->SpellId))
                    player->RemoveSpell(perk->SpellId, false, false);
            }
    }
    void Queue(Player* player)
    {
        if (player)
        {
            std::lock_guard<std::mutex> guard(pendingMutex);
            pending.push_back(player->GetGUID());
        }
    }
    bool ApplyHearthstone(Player* player, uint32 itemId = HearthstoneItem)
    {
        if (!sConfigMgr->GetBoolDefault("SoloQoL.ShortHearthstone", true))
            return false;
        auto duration = QoLPolicy::CooldownMillis(sConfigMgr->GetIntDefault("SoloQoL.HearthstoneCooldownSeconds", 300));
        if (!duration)
            return false;
        auto const& cooldowns = player->GetSpellHistory()->GetCooldowns();
        auto current = cooldowns.find(HearthstoneSpell);
        uint32 category = current != cooldowns.end() ? current->second.CategoryId : 0;
        if (!category)
            if (ItemTemplate const* item = sObjectMgr->GetItemTemplate(itemId))
                for (auto const& effect : item->Spells)
                    if (effect.SpellId == int32(HearthstoneSpell))
                        category = effect.SpellCategory;
        player->GetSpellHistory()->RemoveCooldown(HearthstoneSpell, true);
        if (*duration)
        {
            // Explicit final duration avoids Hasty Hearth's flat discount making short profiles negative.
            player->GetSpellHistory()->AddCooldown(HearthstoneSpell, itemId, *duration, category, *duration);
            player->GetSpellHistory()->SendCooldown(HearthstoneSpell, *duration);
        }
        return true;
    }
    uint32 ServiceFlags(Player* player)
    {
        uint32 flags = 0;
        for (auto flag : { AT_LOGIN_RENAME, AT_LOGIN_CUSTOMIZE, AT_LOGIN_CHANGE_RACE, AT_LOGIN_CHANGE_FACTION })
            if (player->HasAtLoginFlag(flag))
                flags |= uint32(flag);
        return flags;
    }
    bool Service(Player* player, uint32 action)
    {
        uint32 flag = QoLPolicy::ServiceFlag(action);
        if (!flag || !QoLPolicy::CanUseServices(sConfigMgr->GetBoolDefault("SoloQoL.CharacterServices", true),
            player->GetSession()->IsBot(), player->GetRace() == RACE_PANDAREN_NEUTRAL, player->IsInCombat()))
            return false;
        // One service at a time, preserving reset/first-login and all unrelated flags.
        for (auto old : { AT_LOGIN_RENAME, AT_LOGIN_CUSTOMIZE, AT_LOGIN_CHANGE_RACE, AT_LOGIN_CHANGE_FACTION })
            player->RemoveAtLoginFlag(old);
        player->SetAtLoginFlag(static_cast<AtLoginFlags>(flag));
        player->SaveToDB();
        player->GetSession()->SendNotification("Your free service is ready. Log out to use it on character selection.");
        return true;
    }
    Player* Target(ChatHandler* handler)
    {
        Player* player = handler->getSelectedPlayer();
        return player ? player : handler->GetSession()->GetPlayer();
    }
    bool Status(ChatHandler* handler, char const*)
    {
        Player* player = Target(handler);
        uint32 known = 0, total = 0;
        for (uint32 i = 0; i < sGuildPerkSpellsStore.GetNumRows(); ++i)
            if (GuildPerkSpellsEntry const* perk = sGuildPerkSpellsStore.LookupEntry(i))
            {
                ++total;
                if (player->HasSpell(perk->SpellId))
                    ++known;
            }
        handler->PSendSysMessage("SoloQoL: perks-all=%u; perks=%u/%u; services=%u; service-flags=%u; hearth-enabled=%u; hearth-seconds=%d; hearth-remaining-ms=%u; mixed-armor=%u; mixed-weapons=%u",
            uint32(AllPerks()), known, total, uint32(sConfigMgr->GetBoolDefault("SoloQoL.CharacterServices", true)), ServiceFlags(player),
            uint32(sConfigMgr->GetBoolDefault("SoloQoL.ShortHearthstone", true)), sConfigMgr->GetIntDefault("SoloQoL.HearthstoneCooldownSeconds", 300),
            player->GetSpellHistory()->GetCooldownDelay(HearthstoneSpell, HearthstoneItem),
            uint32(sTransmogrification->GetAllowMixedArmorTypes()),
            uint32(sTransmogrification->GetAllowMixedWeaponTypes()));
        return true;
    }
    bool Perks(ChatHandler* handler, char const*) { ReconcilePerks(Target(handler)); return Status(handler, ""); }
    bool Hearth(ChatHandler* handler, char const*) { ApplyHearthstone(Target(handler), HearthstoneItem); return Status(handler, ""); }
    bool ResetHearth(ChatHandler* handler, char const*) { Target(handler)->GetSpellHistory()->RemoveCooldown(HearthstoneSpell, true); return Status(handler, ""); }
    bool ForceService(ChatHandler* handler, char const* args)
    {
        std::string action(args ? args : "");
        uint32 number = action == "rename" ? 1 : action == "customize" ? 2 : action == "race" ? 3 : action == "faction" ? 4 : 0;
        if (!Service(Target(handler), number))
        {
            handler->SendSysMessage("Use rename/customize/race/faction on an eligible player out of combat");
            handler->SetSentErrorMessage(true);
            return false;
        }
        return Status(handler, "");
    }
    class solo_qol_commands : public CommandScript
    {
    public:
        solo_qol_commands() : CommandScript("solo_qol_commands") { }
        Trinity::ChatCommands::ChatCommandTable GetCommands() const override
        {
            using namespace Trinity::ChatCommands;
            static ChatCommandTable commands =
            {
                { "status", &Status, Permission, Console::No },
                { "perks", &Perks, Permission, Console::No },
                { "hearth", &Hearth, Permission, Console::No },
                { "hearthreset", &ResetHearth, Permission, Console::No },
                { "service", &ForceService, Permission, Console::No }
            };
            return { { "soloqol", commands, Permission, Console::No } };
        }
    };
    class npc_soloqol_services : public CreatureScript
    {
    public:
        npc_soloqol_services() : CreatureScript("npc_soloqol_services") { }
        struct ServiceAI : public PassiveAI
        {
            explicit ServiceAI(Creature* creature) : PassiveAI(creature) { Reset(); }
            uint32 RespawnDelay() const
            {
                int32 seconds = sConfigMgr->GetIntDefault("SoloQoL.ServicesRespawnSeconds", 300);
                return seconds > 0 ? uint32(seconds) : 0;
            }
            void Reset() override { me->SetRespawnDelay(RespawnDelay()); }
            void CorpseRemoved(uint32& delay) override { delay = RespawnDelay(); me->SetRespawnDelay(delay); }
        };
        CreatureAI* GetAI(Creature* creature) const override { return new ServiceAI(creature); }
        bool OnGossipHello(Player* player, Creature* creature) override
        {
            if (!QoLPolicy::CanUseServices(sConfigMgr->GetBoolDefault("SoloQoL.CharacterServices", true),
                player->GetSession()->IsBot(), player->GetRace() == RACE_PANDAREN_NEUTRAL, player->IsInCombat()))
                return false;
            ClearGossipMenuFor(player);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Rename my character", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 1);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Customize my appearance", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 2);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Change my race", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 3);
            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Change my faction", GOSSIP_SENDER_MAIN, GOSSIP_ACTION_INFO_DEF + 4);
            SendGossipMenuFor(player, ServicesText, creature->GetGUID());
            return true;
        }
        bool OnGossipSelect(Player* player, Creature*, uint32 sender, uint32 action) override
        {
            CloseGossipMenuFor(player);
            return sender == GOSSIP_SENDER_MAIN && action > GOSSIP_ACTION_INFO_DEF && Service(player, action - GOSSIP_ACTION_INFO_DEF);
        }
    };
    class solo_qol_world : public WorldScript
    {
    public:
        solo_qol_world() : WorldScript("solo_qol_world") { }
        void OnUpdate(uint32) override
        {
            std::vector<ObjectGuid> work;
            {
                std::lock_guard<std::mutex> guard(pendingMutex);
                work.swap(pending);
            }
            for (ObjectGuid guid : work)
                if (Player* player = ObjectAccessor::FindPlayer(guid))
                    ReconcilePerks(player);
        }
        void OnConfigLoad(bool reload) override
        {
            if (reload)
                for (auto const& session : sWorld->GetAllSessions())
                    Queue(session.second->GetPlayer());
        }
    };
    class solo_qol_guild : public GuildScript
    {
    public:
        solo_qol_guild() : GuildScript("solo_qol_guild") { }
        void OnAddMember(Guild*, Player* player, uint8&) override { Queue(player); }
        void OnRemoveMember(Guild*, Player* player, bool, bool) override { Queue(player); }
    };
    class solo_qol_player : public PlayerScript
    {
    public:
        solo_qol_player() : PlayerScript("solo_qol_player") { }
        void OnLogin(Player* player) override
        {
            ReconcilePerks(player);
            if (!player->GetSession()->IsBot() && sConfigMgr->GetBoolDefault("SoloQoL.IntroOnLogin", true))
            {
                std::string intro;
                if (AllPerks())
                    intro = "Guild perks are available to everyone. ";
                if (sConfigMgr->GetBoolDefault("SoloQoL.CharacterServices", true))
                    intro += "City stewards offer free character services. ";
                if (sConfigMgr->GetBoolDefault("SoloQoL.ShortHearthstone", true))
                    if (auto duration = QoLPolicy::CooldownMillis(sConfigMgr->GetIntDefault("SoloQoL.HearthstoneCooldownSeconds", 300)))
                        intro += "Hearthstone cooldown: " + std::to_string(*duration / 1000) + " seconds.";
                if (!intro.empty())
                    player->GetSession()->SendNotification("%s", intro.c_str());
            }
        }
    };
    class spell_soloqol_hearthstone : public SpellScriptLoader
    {
    public:
        spell_soloqol_hearthstone() : SpellScriptLoader("spell_soloqol_hearthstone") { }
        class HearthstoneScript : public SpellScript
        {
            PrepareSpellScript(HearthstoneScript);
            void After()
            {
                // Native modifiers can erase a short item cooldown before this hook.
                // Override completed normal casts even then; core clears GM cheat cooldowns afterwards.
                if (!GetSpell()->IsTriggered())
                    if (Player* player = GetCaster()->ToPlayer())
                        ApplyHearthstone(player, GetCastItem() ? GetCastItem()->GetEntry() : HearthstoneItem);
            }
            void Register() override { AfterCast += SpellCastFn(HearthstoneScript::After); }
        };
        SpellScript* GetSpellScript() const override { return new HearthstoneScript(); }
    };
}
void AddSC_solo_qol()
{
    new solo_qol_commands();
    new npc_soloqol_services();
    new solo_qol_world();
    new solo_qol_guild();
    new solo_qol_player();
    new spell_soloqol_hearthstone();
}
