/*
 * Heartseed state per character (characters DB) and stage/glow data (world DB).
 * See README.md in this folder and docs/DESIGN.md, "The Heartseed".
 */

#ifndef HEARTSEED_MGR_H
#define HEARTSEED_MGR_H

#include "Define.h"
#include "HeartseedRules.h"

#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Player;
class Unit;

namespace Heartseed
{
    // Server-side passive (spell_dbc) carrying the core passive's bonus weapon damage.
    constexpr uint32 SPELL_CORE_WEAPON_BONUS = 310008;

    // Story beats; stored as rows in heartseed_story_flag. Append only: the values are saved.
    enum StoryFlag : uint16
    {
        STORY_FLAG_FIRST_DUNGEON    = 1,
        STORY_FLAG_REACHED_PANDARIA = 2,
        STORY_FLAG_FIRST_WORLD_BOSS = 3,
    };

    struct State
    {
        uint8 Rank = 1;
        uint32 Growth = 0;
        uint32 GrantedTime = 0;
        // spec id -> trait chosen at each major tier (0 = not chosen)
        std::map<uint16, std::array<uint32, MaxMajorTiers>> Loadouts;
        // essence id -> (times collected, active slot or 0)
        std::map<uint32, std::pair<uint32, uint8>> Essences;
        // story flag -> unix time it was set
        std::map<uint16, uint32> StoryFlags;
        bool Dirty = false;

        // Runtime only, never saved: what RefreshTraits last applied to the character.
        std::map<uint32, int32> AppliedAuras;         // spell id -> effect 0 amount, core aura included
        uint16 LastSpecId = 0;                        // spec whose loadout was last applied
        bool HasLastSpec = false;
        uint32 LastProcTime = 0;                      // getMSTime() of the last core proc
        bool HasProcced = false;
        uint32 ProcCount = 0;                         // procs cast since login, for .heartseed info
    };

    struct TraitInfo
    {
        uint32 Id = 0;
        Path TraitPath = Path::None;
        TraitKind Kind = TraitKind::Minor;
        uint8 Tier = 0;                               // majors: major tier index
        uint8 MinRank = 1;                            // minors: rank that fills it in
        uint32 SpellId = 0;                           // passive aura applied while the trait is active
        int32 BaseAmount = 0;                         // effect 0 amount at item level 0
        float AmountPerItemLevel = 0.0f;              // added per main-hand item level
        std::string Name;
        std::string Description;
    };

    enum class ChoiceResult
    {
        Ok,
        NoHeartseed,
        UnknownTrait,
        NotMajor,
        WrongTier,
        TierLocked,                                   // character level below the tier
    };

    struct StageInfo
    {
        uint8 MinLevel = 1;
        std::string Name;
    };
}

class HeartseedMgr
{
public:
    static HeartseedMgr* instance();

    void LoadConfig();
    void LoadData();                       // world: heartseed_stage, heartseed_glow, heartseed_trait
    bool IsEnabled() const { return _enabled; }

    void LoadPlayer(Player* player);       // characters: all heartseed_* rows of one character
    void SavePlayer(Player* player);       // writes state if changed
    void UnloadPlayer(Player* player);

    bool HasHeartseed(Player const* player) const;
    // Copy of the state, so callers never hold a pointer into the map other threads may change.
    bool GetState(Player const* player, Heartseed::State& out) const;
    bool Grant(Player* player);            // false if it already has one
    bool Revoke(Player* player);           // deletes every heartseed_* row of the character

    // State changes for GM commands and later systems (feeding, story hooks). Each saves at once and
    // returns false when the character has no Heartseed.
    bool SetRank(Player* player, uint8 rank);
    bool SetGrowth(Player* player, uint32 growth);
    bool SetStoryFlag(Player* player, uint16 flag, bool set);

    // Known story beats by name ("first_dungeon"); 0 if unknown. Unknown ids are still valid flags.
    static uint16 GetStoryFlagByName(std::string const& name);
    static char const* GetStoryFlagName(uint16 flag);

    // Major choice for the active specialization's loadout. traitId 0 clears the tier. force skips the
    // level check (GM commands); the shrine gossip (T-023) calls it without.
    Heartseed::ChoiceResult ChooseMajor(Player* player, uint8 tier, uint32 traitId, bool force);

    std::vector<Heartseed::TraitInfo> GetTraits() const;
    bool GetTrait(uint32 traitId, Heartseed::TraitInfo& out) const;

    // Brings the character's trait auras and core weapon bonus in line with its state, spec, level and
    // main-hand weapon. Cheap when nothing changed.
    void RefreshTraits(Player* player);
    // Traits plus glow; called after any state change.
    void Refresh(Player* player);

    // Core passive proc: rolled on damage the character deals, dealt one update later.
    void OnDamageDealt(Player* player, Unit* victim);
    void CastProc(Player* player, Unit* victim);
    uint32 GetProcDamage(Player* player) const;
    uint32 GetProcSpellId() const { return _procSpellId; }

    uint8 GetStage(Player const* player) const;
    std::string GetStageName(uint8 stage) const;
    Heartseed::Path GetDominantPath(Player const* player) const;
    uint16 GetGlowEnchant(Player const* player) const;
    void RefreshGlow(Player* player);      // re-sends the main hand's visible item field

private:
    HeartseedMgr() = default;

    bool ModifyState(Player* player, std::function<void(Heartseed::State&)> const& change);
    // Callers hold _lock.
    std::array<uint8, Heartseed::MaxPaths> MajorsPerPath(Heartseed::State const& state, uint16 specId) const;
    static void RemoveAuras(Player* player, std::map<uint32, int32> const& auras);

    bool _enabled = true;
    float _coreWeaponDamagePct = 0.0f;
    uint32 _procSpellId = 0;
    float _procChance = 0.0f;
    float _procWeaponDamagePct = 0.0f;
    uint32 _procCooldown = 0;
    std::vector<Heartseed::StageInfo> _stages;
    std::vector<uint8> _stageMinLevels;
    std::map<std::pair<uint8, uint8>, uint16> _glow;    // (stage, path) -> enchant id
    std::map<uint32, Heartseed::TraitInfo> _traits;     // trait id -> trait

    mutable std::mutex _lock;                           // players save from map threads
    std::unordered_map<uint32, Heartseed::State> _states;   // guid counter -> state
};

#define sHeartseedMgr HeartseedMgr::instance()

#endif
