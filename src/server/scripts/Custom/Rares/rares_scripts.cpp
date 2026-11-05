#include "../Affixes/AffixMgr.h"
#include "Chat.h"
#include "Config.h"
#include "Creature.h"
#include "CreatureHooks.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "Group.h"
#include "Log.h"
#include "LootMgr.h"
#include "Map.h"
#include "ObjectAccessor.h"
#include "Player.h"
#include "RBAC.h"
#include "ScriptMgr.h"
#include "TemporarySummon.h"
#include "WorldSession.h"
#include "../Reload/ReloadRegistry.h"
#include <list>
#include <mutex>
#include <set>
#include <sstream>

// Affixed rares (DESIGN.md, Rares and discovery): open-world rares roll 0-3 affixes from the shared affix module,
// wait for a human before bots may fight them, and pay extra XP and loot for each affix.
namespace
{
    constexpr uint32 SPELL_UNCLAIMED = 318000;
    rbac::RBACPermissions const GmPermission = static_cast<rbac::RBACPermissions>(98000);

    struct Config
    {
        bool Enable = true;
        bool BotsWaitForPlayer = true;
        std::vector<uint32_t> CountWeights = { 30, 40, 22, 8 };
        float XpBase = 0.25f;
        float XpMax = 1.0f;
        bool Intro = true;
    };
    Config config;

    std::vector<uint32_t> ParseWeights(std::string const& text)
    {
        std::vector<uint32_t> weights;
        std::stringstream in(text);
        std::string part;
        while (std::getline(in, part, ','))
        {
            try { weights.push_back(uint32_t(std::stoul(part))); }
            catch (...) { return {}; }
        }
        return weights;
    }

    void LoadConfig()
    {
        config.Enable = sConfigMgr->GetBoolDefault("Rares.Affixes.Enable", true);
        config.BotsWaitForPlayer = sConfigMgr->GetBoolDefault("Rares.BotsWaitForPlayer", true);
        std::vector<uint32_t> weights = ParseWeights(sConfigMgr->GetStringDefault("Rares.Affixes.CountWeights", "30,40,22,8"));
        config.CountWeights = weights.empty() ? std::vector<uint32_t>{ 30, 40, 22, 8 } : weights;
        config.XpBase = std::max(0.0f, sConfigMgr->GetFloatDefault("Rares.Xp.BaseLevelFraction", 0.25f));
        config.XpMax = std::max(0.0f, sConfigMgr->GetFloatDefault("Rares.Xp.MaxLevelFraction", 1.0f));
        config.Intro = sConfigMgr->GetBoolDefault("Rares.IntroOnFirstClaim", true);
    }

    bool IsOpenWorldRare(Creature const* creature)
    {
        if (!creature || creature->IsPet() || creature->IsTotem() || creature->GetMap()->Instanceable())
            return false;
        uint32 rank = creature->GetCreatureTemplate()->rank;
        return rank == CREATURE_ELITE_RARE || rank == CREATURE_ELITE_RAREELITE;
    }

    bool IsBotSide(Unit const* unit)
    {
        Unit const* owner = unit->GetCharmerOrOwnerOrSelf();
        Player const* player = owner ? owner->ToPlayer() : nullptr;
        return player && player->GetSession() && player->GetSession()->IsBot();
    }

    Player* HumanBehind(Unit* unit)
    {
        Unit* owner = unit ? unit->GetCharmerOrOwnerOrSelf() : nullptr;
        Player* player = owner ? owner->ToPlayer() : nullptr;
        return player && player->GetSession() && !player->GetSession()->IsBot() ? player : nullptr;
    }

    void RollAffixes(Creature* creature, int32 forcedCount = -1)
    {
        uint32 count = forcedCount >= 0 ? uint32(forcedCount) : Affixes::RollCount(config.CountWeights,
            [](uint64_t n) { return n <= 1 ? uint64_t(0) : uint64_t(urand(0, uint32(n - 1))); });
        sAffixMgr->Apply(creature, sAffixMgr->Roll(Affixes::HOST_RARE, count));
    }

    void Hold(Creature* creature)
    {
        if (config.BotsWaitForPlayer && !creature->HasAura(SPELL_UNCLAIMED))
            creature->AddAura(SPELL_UNCLAIMED, creature);
    }

    // First claim per character this server run shows how affixed rares work. (Interim: kept in memory; the
    // journal, Phase 1b, takes over.)
    std::mutex introLock;
    std::set<ObjectGuid> introShown;

    void Claim(Creature* creature, Player* player)
    {
        creature->RemoveAurasDueToSpell(SPELL_UNCLAIMED);
        std::vector<uint32_t> affixes = sAffixMgr->Get(creature->GetGUID());
        if (affixes.empty())
            return;
        ChatHandler(player->GetSession()).PSendSysMessage("|cffff8000%s|r is %s. Each affix adds XP and loot.",
            creature->GetName().c_str(), sAffixMgr->Describe(affixes).c_str());
        if (!config.Intro)
            return;
        std::lock_guard<std::mutex> guard(introLock);
        if (introShown.insert(player->GetGUID()).second)
            player->GetSession()->SendNotification("Affixed rare! Its buffs are its powers. Bots leave it to you until you strike first.");
    }

    void OnCreatureEvent(Creature* creature, CreatureHooks::Event event)
    {
        switch (event)
        {
            case CreatureHooks::Event::Spawned:
            case CreatureHooks::Event::Respawned:
                if (!config.Enable || !IsOpenWorldRare(creature))
                    return;
                RollAffixes(creature);
                Hold(creature);
                break;
            case CreatureHooks::Event::Evaded:
                if (sAffixMgr->Get(creature->GetGUID()).empty())
                    return;
                sAffixMgr->Reapply(creature);
                Hold(creature);
                break;
            case CreatureHooks::Event::Removed:
                sAffixMgr->Forget(creature->GetGUID());
                break;
        }
    }

    // Bots and an unclaimed rare don't fight in either direction. Reads only the two units: safe on map threads.
    bool BlocksBotFight(Creature const* creature, Unit const* other)
    {
        return creature->HasAura(SPELL_UNCLAIMED) && IsBotSide(other);
    }

    void PayKill(Player* killer, Creature* killed)
    {
        std::vector<uint32_t> affixes = sAffixMgr->Get(killed->GetGUID());
        if (affixes.empty() && !IsOpenWorldRare(killed))
            return;

        std::vector<uint32_t> bonus;
        uint32 lootRolls = 0;
        for (uint32 id : affixes)
            if (Affixes::Template const* t = sAffixMgr->Find(id))
            {
                bonus.push_back(t->XpBonusPct);
                lootRolls += t->LootRolls;
            }
        float fraction = Affixes::RareXpFraction(config.XpBase, bonus, config.XpMax);

        std::vector<Player*> earners{ killer };
        if (Group* group = killer->GetGroup())
            for (GroupReference* ref = group->GetFirstMember(); ref; ref = ref->next())
                if (Player* member = ref->GetSource())
                    if (member != killer && member->IsAlive() && member->IsAtGroupRewardDistance(killed))
                        earners.push_back(member);
        for (Player* player : earners)
            if (uint32 xp = uint32(fraction * player->GetUInt32Value(PLAYER_FIELD_NEXT_LEVEL_XP)))
                player->GiveXP(xp, killed);

        uint32 lootId = killed->GetCreatureTemplate()->lootid;
        if (LootTemplate const* table = lootId ? LootTemplates_Creature.GetLootFor(lootId) : nullptr)
            for (uint32 i = 0; i < lootRolls; ++i)
                table->Process(killed->loot, true, 1 << killed->GetMap()->GetDifficulty(), 0, killer);
        if (lootRolls)
            killed->loot.gold = uint32(killed->loot.gold * (1 + lootRolls));
    }
}

class rares_unitscript : public UnitScript
{
public:
    rares_unitscript() : UnitScript("rares_unitscript") { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& /*damage*/) override
    {
        Creature* creature = victim ? victim->ToCreature() : nullptr;
        if (!creature || !creature->HasAura(SPELL_UNCLAIMED))
            return;
        if (Player* player = HumanBehind(attacker))
            Claim(creature, player);
    }
};

class rares_playerscript : public PlayerScript
{
public:
    rares_playerscript() : PlayerScript("rares_playerscript") { }

    void OnCreatureKill(Player* killer, Creature* killed) override
    {
        if (config.Enable)
            PayKill(killer, killed);
    }
};

class rares_worldscript : public WorldScript
{
public:
    rares_worldscript() : WorldScript("rares_worldscript") { }

    void OnConfigLoad(bool /*reload*/) override { LoadConfig(); }
};

class rares_commandscript : public CommandScript
{
public:
    rares_commandscript() : CommandScript("rares_commandscript") { }

    std::vector<ChatCommand> GetCommands() const override
    {
        static std::vector<ChatCommand> rareTable =
        {
            { "near",   &HandleNear,   GmPermission, Trinity::ChatCommands::Console::No },
            { "spawn",  &HandleSpawn,  GmPermission, Trinity::ChatCommands::Console::No },
            { "reroll", &HandleReroll, GmPermission, Trinity::ChatCommands::Console::No },
            { "set",    &HandleSet,    GmPermission, Trinity::ChatCommands::Console::No },
            { "claim",  &HandleClaim,  GmPermission, Trinity::ChatCommands::Console::No },
            { "affixes",&HandleList,   GmPermission, Trinity::ChatCommands::Console::Yes },
        };
        static std::vector<ChatCommand> commandTable =
        {
            { "rare", rareTable, GmPermission, Trinity::ChatCommands::Console::Yes },
        };
        return commandTable;
    }

    // The selected creature, or the nearest affixed or rare creature within 50 yd.
    static Creature* Pick(ChatHandler* handler)
    {
        if (Creature* selected = handler->getSelectedCreature())
            return selected;
        Player* player = handler->GetSession()->GetPlayer();
        std::list<Creature*> creatures;
        Trinity::AnyUnitInObjectRangeCheck check(player, 50.0f);
        Trinity::CreatureListSearcher<Trinity::AnyUnitInObjectRangeCheck> searcher(player, creatures, check);
        player->VisitNearbyGridObject(50.0f, searcher);
        Creature* best = nullptr;
        for (Creature* c : creatures)
            if (c->IsAlive() && (IsOpenWorldRare(c) || !sAffixMgr->Get(c->GetGUID()).empty())
                && (!best || player->GetDistance(c) < player->GetDistance(best)))
                best = c;
        return best;
    }

    static void Report(ChatHandler* handler, Creature* c)
    {
        handler->PSendSysMessage("Rare %s (entry %u, guid %s): affixes %s; %s; health %u",
            c->GetName().c_str(), c->GetEntry(), c->GetGUID().ToString().c_str(),
            sAffixMgr->Describe(sAffixMgr->Get(c->GetGUID())).c_str(),
            c->HasAura(SPELL_UNCLAIMED) ? "unclaimed (bots wait)" : "claimed", c->GetMaxHealth());
    }

    static bool HandleNear(ChatHandler* handler, char const* /*args*/)
    {
        if (Creature* c = Pick(handler))
        {
            Report(handler, c);
            return true;
        }
        handler->SendSysMessage("No rare within 50 yd.");
        return true;
    }

    // .rare spawn <entry> [affix names...]: a temporary rare at your position with those affixes (or a roll).
    static bool HandleSpawn(ChatHandler* handler, char const* args)
    {
        std::stringstream in(args ? args : "");
        uint32 entry = 0;
        if (!(in >> entry) || !entry)
        {
            handler->SendSysMessage("Use .rare spawn <entry> [affix ...]");
            return false;
        }
        std::vector<uint32_t> affixes;
        std::string name;
        while (in >> name)
        {
            Affixes::Template const* t = sAffixMgr->FindByName(name);
            if (!t)
            {
                handler->PSendSysMessage("Unknown affix '%s' (see .rare affixes)", name.c_str());
                return false;
            }
            affixes.push_back(t->Id);
        }
        Player* player = handler->GetSession()->GetPlayer();
        Position pos = player->GetNearPosition(5.0f, 0.0f);
        TempSummon* c = player->SummonCreature(entry, pos, TEMPSUMMON_CORPSE_TIMED_DESPAWN, 120000ms);
        if (!c)
        {
            handler->PSendSysMessage("Could not spawn creature %u", entry);
            return false;
        }
        c->SetFaction(14);  // hostile to everyone, so a test rare never sides with the GM
        if (!affixes.empty())
            sAffixMgr->Apply(c, affixes);
        else if (sAffixMgr->Get(c->GetGUID()).empty())
            RollAffixes(c, 1);
        Hold(c);
        Report(handler, c);
        return true;
    }

    static bool HandleReroll(ChatHandler* handler, char const* args)
    {
        Creature* c = Pick(handler);
        if (!c)
            return false;
        int32 count = args && *args ? std::max(0, atoi(args)) : -1;
        RollAffixes(c, std::min(count, 3));
        Hold(c);
        Report(handler, c);
        return true;
    }

    static bool HandleSet(ChatHandler* handler, char const* args)
    {
        Creature* c = Pick(handler);
        if (!c)
            return false;
        std::stringstream in(args ? args : "");
        std::vector<uint32_t> affixes;
        std::string name;
        while (in >> name)
            if (Affixes::Template const* t = sAffixMgr->FindByName(name))
                affixes.push_back(t->Id);
        sAffixMgr->Apply(c, affixes);
        Report(handler, c);
        return true;
    }

    static bool HandleClaim(ChatHandler* handler, char const* /*args*/)
    {
        Creature* c = Pick(handler);
        if (!c)
            return false;
        Claim(c, handler->GetSession()->GetPlayer());
        Report(handler, c);
        return true;
    }

    static bool HandleList(ChatHandler* handler, char const* /*args*/)
    {
        auto templates = sAffixMgr->Templates();
        for (auto const& entry : *templates)
        {
            Affixes::Template const& t = entry.second;
            handler->PSendSysMessage("Affix %u %s: kind %u, aura %u, every %u ms, amount %.1f%%, +%u%% XP, +%u loot, weight %u, hosts 0x%X",
                t.Id, t.Name.c_str(), uint32(t.AffixKind), t.AuraSpell, t.PeriodMs, t.AmountPct, t.XpBonusPct, t.LootRolls, t.Weight, t.Hosts);
        }
        handler->PSendSysMessage("%u affixes", uint32(templates->size()));
        return true;
    }
};

void AddSC_affixes();

void AddSC_rares()
{
    AddSC_affixes();
    LoadConfig();
    std::size_t rows = 0;
    sAffixMgr->Load(&rows);
    TC_LOG_INFO("server.loading", ">> Loaded %u affixes", uint32(rows));
    CreatureHooks::Register(&OnCreatureEvent);
    CreatureHooks::RegisterFightFilter(&BlocksBotFight);
    if (!CustomReload::Tables().Register("affix_template", "Affixes (rares now; corruption and Brawler later)", []
    {
        std::size_t rows = 0;
        bool success = sAffixMgr->Load(&rows);
        return CustomReload::Result{ success, rows, "Affixes published; spawns roll from them at their next respawn" };
    }))
        TC_LOG_ERROR("server", "Affix reload loader registration failed");
    new rares_unitscript();
    new rares_playerscript();
    new rares_worldscript();
    new rares_commandscript();
}
