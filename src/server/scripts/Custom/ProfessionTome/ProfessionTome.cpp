#include "ProfessionTomeRules.h"
#include "BattlePet.h"
#include "BattlePetMgr.h"
#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "FirstLoginHooks.h"
#include "Log.h"
#include "ObjectMgr.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"
#include "TrainerListPacket.h"
#include "WorldSession.h"

namespace
{
    // RBAC id from the profession tome block in docs/ID_RANGES.md.
    rbac::RBACPermissions const RBAC_PERM_COMMAND_PROFTOME = static_cast<rbac::RBACPermissions>(101000);

    bool Enabled() { return sConfigMgr->GetBoolDefault("ProfessionTome.Enable", true); }

    BattlePet* FindTome(Player* player)
    {
        for (BattlePet* pet : player->GetBattlePetMgr().BattlePets)
            if (pet->GetSpecies() == ProfessionTome::Species && pet->GetDbState() != BATTLE_PET_DB_STATE_DELETE)
                return pet;
        return nullptr;
    }

    // The journal is per account, so one grant covers every character on it; the summon spell is
    // relearned from the journal at each login (Player::LoadFromDB).
    bool Grant(Player* player)
    {
        if (!FindTome(player))
        {
            player->GetBattlePetMgr().Create(ProfessionTome::Species, 1);
            if (!FindTome(player))
            {
                TC_LOG_ERROR("scripts", "Profession tome: could not add species %u for player %u", ProfessionTome::Species, player->GetGUID().GetCounter());
                return false;
            }
        }
        if (!player->HasSpell(ProfessionTome::SummonSpell))
            player->LearnSpell(ProfessionTome::SummonSpell, false);
        return true;
    }

    void GrantOnFirstLogin(Player* player)
    {
        if (player->GetSession()->IsBot() || !Enabled() || !sConfigMgr->GetBoolDefault("ProfessionTome.GrantOnFirstLogin", true))
            return;
        if (FindTome(player))
            return;
        if (Grant(player))
            player->GetSession()->SendNotification("A Gusting Grimoire joined your pet journal. Summon it to train any profession, anywhere.");
    }

    Creature* CurrentTome(Player* player)
    {
        TempSummon* summon = player->GetBattlePetMgr().GetCurrentSummon();
        return summon && summon->GetEntry() == ProfessionTome::Creature ? summon : nullptr;
    }

    // Same steps as CMSG_SUMMON_BATTLE_PET_COMPANION, so the GM command exercises the journal path.
    bool Summon(Player* player)
    {
        BattlePet* pet = FindTome(player);
        if (!pet)
            return false;
        if (CurrentTome(player))
            return true;
        BattlePetMgr& mgr = player->GetBattlePetMgr();
        mgr.UnSummonCurrentBattlePet(false);
        mgr.SetCurrentSummonId(pet->GetId());
        player->CastSpell(player, ProfessionTome::SummonSpell, true);
        return CurrentTome(player) != nullptr;
    }

    // One profession's slice of the tome's merged list. Buying validates against the full list.
    TrainerSpellData Slice(uint32 skill)
    {
        TrainerSpellData slice;
        slice.trainerType = 2;
        if (TrainerSpellData const* all = sObjectMgr->GetNpcTrainerSpells(ProfessionTome::Creature))
            for (auto const& entry : all->spellList)
                if (entry.second.reqSkill == skill)
                    slice.spellList.emplace(entry.first, entry.second);
        return slice;
    }

    class npc_profession_tome : public CreatureScript
    {
    public:
        npc_profession_tome() : CreatureScript("npc_profession_tome") { }

        bool OnGossipHello(Player* player, Creature* creature) override
        {
            ClearGossipMenuFor(player);
            if (!Enabled())
            {
                CloseGossipMenuFor(player);
                return true;
            }
            for (auto const& profession : ProfessionTome::Professions)
                AddGossipItemFor(player, GOSSIP_ICON_TRAINER, std::string("Teach me ") + profession.Name, GOSSIP_SENDER_MAIN, profession.Skill);
            SendGossipMenuFor(player, DEFAULT_GOSSIP_MESSAGE, creature->GetGUID());
            return true;
        }

        bool OnGossipSelect(Player* player, Creature* creature, uint32 /*sender*/, uint32 action) override
        {
            CloseGossipMenuFor(player);
            ProfessionTome::Profession const* profession = ProfessionTome::Find(action);
            if (!Enabled() || !profession)
                return true;
            TrainerListPacket::Send(player->GetSession(), creature, profession->Name, Slice(profession->Skill));
            return true;
        }
    };

    Player* Target(ChatHandler* handler)
    {
        Player* player = handler->getSelectedPlayer();
        return player ? player : handler->GetSession()->GetPlayer();
    }

    bool Status(ChatHandler* handler, char const*)
    {
        Player* player = Target(handler);
        TrainerSpellData const* all = sObjectMgr->GetNpcTrainerSpells(ProfessionTome::Creature);
        handler->PSendSysMessage("Profession tome: enabled=%u; owned=%u; spell=%u; summoned=%u; trainer spells=%u",
            uint32(Enabled()), uint32(FindTome(player) != nullptr), uint32(player->HasSpell(ProfessionTome::SummonSpell)),
            uint32(CurrentTome(player) != nullptr), all ? uint32(all->spellList.size()) : 0u);
        for (auto const& profession : ProfessionTome::Professions)
            handler->PSendSysMessage("Profession tome list: %s skill=%u; spells=%u", profession.Name, profession.Skill,
                uint32(Slice(profession.Skill).spellList.size()));
        return true;
    }

    bool GrantCommand(ChatHandler* handler, char const*)
    {
        Player* player = Target(handler);
        bool ok = Grant(player);
        handler->PSendSysMessage("Profession tome: granted=%u; owned=%u", uint32(ok), uint32(FindTome(player) != nullptr));
        return true;
    }

    bool SummonCommand(ChatHandler* handler, char const*)
    {
        Player* player = Target(handler);
        bool ok = Summon(player);
        Creature* tome = CurrentTome(player);
        handler->PSendSysMessage("Profession tome: summoned=%u; entry=%u", uint32(ok), tome ? tome->GetEntry() : 0u);
        return true;
    }

    bool CheckCommand(ChatHandler* handler, char const* args)
    {
        uint32 spell = args ? uint32(strtoul(args, nullptr, 10)) : 0;
        if (!spell)
        {
            handler->SendSysMessage("Usage: .proftome check <spell id>");
            handler->SetSentErrorMessage(true);
            return false;
        }
        Player* player = Target(handler);
        TrainerSpellData const* all = sObjectMgr->GetNpcTrainerSpells(ProfessionTome::Creature);
        handler->PSendSysMessage("Profession tome: spell=%u; listed=%u; known=%u", spell,
            uint32(all && all->Find(spell)), uint32(player->HasSpell(spell)));
        return true;
    }

    class profession_tome_commands : public CommandScript
    {
    public:
        profession_tome_commands() : CommandScript("profession_tome_commands") { }
        Trinity::ChatCommands::ChatCommandTable GetCommands() const override
        {
            using namespace Trinity::ChatCommands;
            static ChatCommandTable commands =
            {
                { "status", &Status, RBAC_PERM_COMMAND_PROFTOME, Console::No },
                { "grant", &GrantCommand, RBAC_PERM_COMMAND_PROFTOME, Console::No },
                { "summon", &SummonCommand, RBAC_PERM_COMMAND_PROFTOME, Console::No },
                { "check", &CheckCommand, RBAC_PERM_COMMAND_PROFTOME, Console::No }
            };
            return { { "proftome", commands, RBAC_PERM_COMMAND_PROFTOME, Console::No } };
        }
    };
}

void AddSC_profession_tome()
{
    if (!FirstLoginHooks::Register(&GrantOnFirstLogin))
        TC_LOG_ERROR("scripts", "Profession tome: first-login callback registration failed");
    new npc_profession_tome();
    new profession_tome_commands();
}
