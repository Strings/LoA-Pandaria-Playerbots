#include "RespecPolicy.h"
#include "RespecConfig.h"
#include "Chat.h"
#include "DBCStores.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "World.h"
#include "WorldSession.h"
#include <sstream>

namespace
{
    void Unlock(Player* player)
    {
        if (!RespecPolicy::ShouldUnlock(player->GetLevel(), sWorld->getIntConfig(CONFIG_MIN_DUALSPEC_LEVEL),
            player->GetSpecsCount(), RespecConfig::AutoDualSpec(), player->GetSession()->IsBot()))
            return;
        // Use the same learning effects as the existing dual-specialization trainer.
        player->CastSpell(player, 63680, true);
        player->CastSpell(player, 63624, true);
        player->GetSession()->SendNotification("You can now switch between two specializations.");
    }
    class free_respec_playerscript : public PlayerScript
    {
    public:
        free_respec_playerscript() : PlayerScript("free_respec_playerscript") { }
        void OnLogin(Player* player) override { Unlock(player); }
        void OnLevelChanged(Player* player, uint8 /*oldLevel*/) override { Unlock(player); }
    };
    Player* Target(ChatHandler* handler)
    {
        if (Player* player = handler->getSelectedPlayer())
            return player;
        return handler->GetSession()->GetPlayer();
    }
    bool Info(ChatHandler* handler, char const* args)
    {
        if (args && *args)
            return false;
        Player* player = Target(handler);
        handler->PSendSysMessage("Respec: free gold=%u; free reagents=%u; auto dual=%u; minimum=%u; level=%u; specs=%u; active=%u; talent cost=%u; spec cost=%u",
            uint32(sWorld->getBoolConfig(CONFIG_NO_RESET_TALENT_COST)), uint32(RespecConfig::FreeReagents()), uint32(RespecConfig::AutoDualSpec()),
            sWorld->getIntConfig(CONFIG_MIN_DUALSPEC_LEVEL), uint32(player->GetLevel()), uint32(player->GetSpecsCount()), uint32(player->GetActiveSpec()),
            player->GetNextResetTalentsCost(), player->GetNextResetSpecializationCost());
        for (uint32 spellId : { 113873u, 127649u, 127650u, 89964u, 90647u, 111621u, 686u })
            if (SpellInfo const* spell = sSpellMgr->GetSpellInfo(spellId))
                handler->PSendSysMessage("Respec spell %u: no reagents=%u", spellId, uint32(player->CanNoReagentCast(spell)));
        for (uint8 slot = 0; slot < MAX_GLYPH_SLOT_INDEX; ++slot)
            handler->PSendSysMessage("Glyph slot %u=%u", slot, player->GetGlyph(player->GetActiveSpec(), slot));
        return true;
    }
    bool Reset(ChatHandler* handler, char const* args)
    {
        std::string mode(args ? args : "");
        if (mode != "talents" && mode != "spec")
            return false;
        Player* player = Target(handler);
        uint64 before = player->GetMoney();
        bool done = player->ResetTalents(false, mode == "talents", mode == "spec");
        player->SendTalentsInfoData();
        handler->PSendSysMessage("Respec reset: success=%u; gold charged=%u", uint32(done), uint32(before - player->GetMoney()));
        return true;
    }
    bool Learn(ChatHandler* handler, char const* args)
    {
        std::istringstream input(args ? args : "");
        uint32 talent; std::string extra;
        if (!(input >> talent) || (input >> extra) || !sTalentStore.LookupEntry(talent))
            return false;
        Player* player = Target(handler);
        bool learned = player->LearnTalent(talent);
        player->SendTalentsInfoData();
        handler->PSendSysMessage("Respec talent %u: learned=%u; known=%u", talent, uint32(learned), uint32(player->HasSpell(sTalentStore.LookupEntry(talent)->SpellID)));
        return true;
    }
    bool Cast(ChatHandler* handler, char const* args)
    {
        std::istringstream input(args ? args : "");
        uint32 spellId, index; std::string extra;
        if (!(input >> spellId >> index) || (input >> extra))
            return false;
        SpellInfo const* info = sSpellMgr->GetSpellInfo(spellId);
        if (!info || !(info->HasEffect(SPELL_EFFECT_REMOVE_TALENT) || info->HasEffect(SPELL_EFFECT_APPLY_GLYPH) || info->HasEffect(SPELL_EFFECT_TALENT_SPEC_SELECT)))
            return false;
        Player* player = Target(handler);
        SpellCastTargets targets;
        targets.SetUnitTarget(player);
        Spell* spell = new Spell(player, info, TRIGGERED_NONE);
        spell->m_glyphIndex = index;
        spell->prepare(&targets);
        handler->PSendSysMessage("Respec requested normal cast %u with index %u", spellId, index);
        return true;
    }
    class free_respec_commandscript : public CommandScript
    {
    public:
        free_respec_commandscript() : CommandScript("free_respec_commandscript") { }
        std::vector<ChatCommand> GetCommands() const override
        {
            using Trinity::ChatCommands::Console;
            std::vector<ChatCommand> children =
            {
                { "info", &Info, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Console::No },
                { "reset", &Reset, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Console::No },
                { "learn", &Learn, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Console::No },
                { "cast", &Cast, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Console::No }
            };
            return { { "respec", children, rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG, Console::No } };
        }
    };
}
void AddSC_free_respec()
{
    new free_respec_playerscript();
    new free_respec_commandscript();
}
