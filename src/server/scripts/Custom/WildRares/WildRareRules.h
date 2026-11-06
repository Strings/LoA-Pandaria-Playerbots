#ifndef ULTIMATEMOP_WILD_RARE_RULES_H
#define ULTIMATEMOP_WILD_RARE_RULES_H
// Server-independent wild rare rules: eligibility, the spawn roll with its zone cap and spawn-point cooldown, and
// reward math. Unit-tested in tests/wild_rares.
#include <algorithm>
#include <cstdint>
#include <map>
#include <utility>

namespace WildRares
{
    // What the server knows about a creature, gathered once so the decision is pure.
    struct Facts
    {
        bool IsPlayerControlled = false;  // pet, guardian, charmed, totem or summon of any kind
        bool HasSpawnId = false;          // a database spawn point (summons have none)
        bool InInstance = false;
        bool InCity = false;              // capital, sanctuary or city area flags on its zone or area
        bool HasNpcRole = false;          // any NPC flag (vendor, trainer, quest giver, flight master...) or a gossip menu
        bool NormalRank = false;          // rank 0: not elite, rare, rare elite or boss
        bool Critter = false;             // critters, non-combat pets, totems, triggers
        bool Attackable = false;          // no non-attackable, not-selectable or immune-to-players flags
        bool Hostile = false;             // hostile to players, or neutral (when neutral counts)
        bool Denied = false;              // rare_wild_denylist
        bool Alive = false;
    };

    enum class Verdict : uint8_t
    {
        Eligible,
        NoSpawnPoint,
        PlayerControlled,
        Instance,
        City,
        NpcRole,
        NotNormalRank,
        Critter,
        NotAttackable,
        NotHostile,
        Denied,
        Dead
    };

    inline Verdict Judge(Facts const& f)
    {
        if (f.IsPlayerControlled) return Verdict::PlayerControlled;
        if (!f.HasSpawnId)        return Verdict::NoSpawnPoint;
        if (f.InInstance)         return Verdict::Instance;
        if (f.InCity)             return Verdict::City;
        if (f.HasNpcRole)         return Verdict::NpcRole;
        if (!f.NormalRank)        return Verdict::NotNormalRank;
        if (f.Critter)            return Verdict::Critter;
        if (!f.Attackable)        return Verdict::NotAttackable;
        if (!f.Hostile)           return Verdict::NotHostile;
        if (f.Denied)             return Verdict::Denied;
        if (!f.Alive)             return Verdict::Dead;
        return Verdict::Eligible;
    }

    inline char const* Describe(Verdict v)
    {
        switch (v)
        {
            case Verdict::Eligible:         return "eligible";
            case Verdict::NoSpawnPoint:     return "no spawn point (summoned)";
            case Verdict::PlayerControlled: return "pet, summon or player-controlled";
            case Verdict::Instance:         return "in an instance";
            case Verdict::City:             return "in a city or sanctuary";
            case Verdict::NpcRole:          return "an NPC (flags or gossip)";
            case Verdict::NotNormalRank:    return "elite, rare or boss";
            case Verdict::Critter:          return "critter, trigger or non-combat";
            case Verdict::NotAttackable:    return "not attackable";
            case Verdict::NotHostile:       return "not hostile";
            case Verdict::Denied:           return "on the denylist";
            case Verdict::Dead:             return "dead";
        }
        return "?";
    }

    enum class RollResult : uint8_t
    {
        Wild,
        Missed,      // the chance roll failed
        ZoneFull,    // the zone already has its cap of live wild rares
        Cooldown     // this spawn point turned wild too recently
    };

    // One spawn's roll. `roll` is uniform in [0, 100). Cap and cooldown are checked first so they cost no roll.
    inline RollResult RollSpawn(float chancePct, uint32_t zoneLive, uint32_t zoneCap, bool onCooldown, float roll)
    {
        if (zoneCap && zoneLive >= zoneCap)
            return RollResult::ZoneFull;
        if (onCooldown)
            return RollResult::Cooldown;
        return roll < std::clamp(chancePct, 0.0f, 100.0f) ? RollResult::Wild : RollResult::Missed;
    }

    // Spawn points that turned wild recently: spawn id -> time the cooldown ends.
    class Cooldowns
    {
    public:
        void Start(uint64_t spawnId, int64_t now, int64_t seconds) { _until[spawnId] = now + std::max<int64_t>(0, seconds); }
        bool Active(uint64_t spawnId, int64_t now) const
        {
            auto itr = _until.find(spawnId);
            return itr != _until.end() && itr->second > now;
        }
        int64_t Remaining(uint64_t spawnId, int64_t now) const
        {
            auto itr = _until.find(spawnId);
            return itr == _until.end() ? 0 : std::max<int64_t>(0, itr->second - now);
        }
        // Drops expired entries so the map stays small on a long-running server.
        void Prune(int64_t now)
        {
            for (auto itr = _until.begin(); itr != _until.end();)
                itr = itr->second <= now ? _until.erase(itr) : std::next(itr);
        }
        std::size_t Size() const { return _until.size(); }

    private:
        std::map<uint64_t, int64_t> _until;
    };

    // Live wild rares per (map, zone).
    class ZoneTally
    {
    public:
        using Key = std::pair<uint32_t, uint32_t>;
        uint32_t Live(uint32_t map, uint32_t zone) const
        {
            auto itr = _live.find({ map, zone });
            return itr == _live.end() ? 0 : itr->second;
        }
        void Add(uint32_t map, uint32_t zone) { ++_live[{ map, zone }]; }
        void Remove(uint32_t map, uint32_t zone)
        {
            auto itr = _live.find({ map, zone });
            if (itr == _live.end())
                return;
            if (--itr->second == 0)
                _live.erase(itr);
        }
        uint32_t Total() const
        {
            uint32_t total = 0;
            for (auto const& entry : _live)
                total += entry.second;
            return total;
        }
        std::map<Key, uint32_t> const& All() const { return _live; }

    private:
        std::map<Key, uint32_t> _live;
    };

    // Extra health as a percent of base, picked uniformly in [min, max]; `unit` is uniform in [0, 1].
    inline uint32_t HealthBonusPct(uint32_t minPct, uint32_t maxPct, float unit)
    {
        if (maxPct < minPct)
            std::swap(minPct, maxPct);
        float span = float(maxPct - minPct);
        return minPct + uint32_t(std::clamp(unit, 0.0f, 1.0f) * span + 0.5f);
    }

    // Kill XP as a fraction of the killer's level: a base share plus a share per affix, capped.
    inline float XpFraction(float base, float perAffix, uint32_t affixes, float max)
    {
        return std::clamp(base + perAffix * float(affixes), 0.0f, std::max(0.0f, max));
    }
}
#endif
