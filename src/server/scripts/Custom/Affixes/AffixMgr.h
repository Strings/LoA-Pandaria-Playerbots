#ifndef ULTIMATEMOP_AFFIX_MGR_H
#define ULTIMATEMOP_AFFIX_MGR_H
#include "AffixRules.h"
#include "ObjectGuid.h"
#include <cstddef>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Creature;
class Unit;

namespace Affixes
{
    // What an affix does. Numbers are stored in affix_template.kind.
    enum class Kind : uint8_t
    {
        Molten      = 1,  // fire nova around it every period; explodes on death
        Frozen      = 2,  // frost nova (damage and root) every period
        Vampiric    = 3,  // heals for a share of the damage it deals
        Shielded    = 4,  // absorb shield on itself every period
        Teleporting = 5,  // blinks behind its victim every period
        Juggernaut  = 6,  // more health, bigger, slower, can't be rooted, snared or stunned
        Summoner    = 7,  // calls adds every period
        ShaTouched  = 8   // fears everyone close every period
    };

    struct Template
    {
        uint32_t Id = 0;
        std::string Name;
        Kind AffixKind = Kind::Molten;
        uint32_t AuraSpell = 0;     // the visible marker aura (clientpatch clone, 318001+)
        uint32_t SpellId = 0;       // client spell the mechanic casts, if any
        uint32_t PeriodMs = 0;
        float AmountPct = 0.0f;     // share of health (victim's for damage, its own for shields and heals)
        uint32_t SummonEntry = 0;
        uint32_t SummonCount = 0;
        uint32_t XpBonusPct = 0;    // added to the kill's share of a level
        uint32_t LootRolls = 0;     // extra rolls of its loot table
        uint32_t Weight = 0;
        uint32_t Hosts = 0;         // HostMask
    };

    class Manager
    {
    public:
        static Manager* Instance();

        bool Load(std::size_t* rows);
        std::shared_ptr<std::map<uint32_t, Template> const> Templates() const;
        Template const* Find(uint32_t id) const;
        Template const* FindByName(std::string const& name) const;
        Template const* FindByAura(uint32_t auraSpell) const;
        std::vector<uint32_t> Roll(uint32_t host, uint32_t count) const;

        // Applies the affixes and remembers them, so an evade can put them back and the kill can pay for them.
        // `intensity` scales them (1 = full strength; AffixRules.h); `host` says which system applied them.
        void Apply(Creature* creature, std::vector<uint32_t> const& affixes, float intensity = 1.0f, uint32_t host = HOST_RARE);
        void Reapply(Creature* creature);
        void Clear(Creature* creature);
        void Forget(ObjectGuid guid);
        std::vector<uint32_t> Get(ObjectGuid guid) const;
        float Intensity(ObjectGuid guid) const;   // 1 when none applied
        uint32_t Host(ObjectGuid guid) const;     // 0 when none applied
        std::string Describe(std::vector<uint32_t> const& affixes) const;

    private:
        struct Applied
        {
            std::vector<uint32_t> Ids;
            float Intensity = 1.0f;
            uint32_t Host = HOST_RARE;
        };
        mutable std::mutex _lock;
        std::shared_ptr<std::map<uint32_t, Template> const> _templates = std::make_shared<std::map<uint32_t, Template>>();
        std::unordered_map<ObjectGuid, Applied> _applied;
        std::vector<std::shared_ptr<std::map<uint32_t, Template> const>> _retired;
    };
}
#define sAffixMgr Affixes::Manager::Instance()
#endif
