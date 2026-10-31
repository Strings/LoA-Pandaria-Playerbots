/*
 * Heartseed state per character (characters DB) and stage/glow data (world DB).
 * See README.md in this folder and docs/DESIGN.md, "The Heartseed".
 */

#ifndef HEARTSEED_MGR_H
#define HEARTSEED_MGR_H

#include "Define.h"
#include "HeartseedRules.h"
#include "ObjectGuid.h"

#include <functional>
#include <map>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>
#include <vector>

class Creature;
class Item;
class Quest;
class Player;
class Unit;
struct ItemTemplate;

namespace Heartseed
{
    // Server-side passive (spell_dbc) carrying the core passive's bonus weapon damage.
    constexpr uint32 SPELL_CORE_WEAPON_BONUS = 310008;

    // Gossip the Heartseed opens from the player itself to ask before it feeds (Heartseed gossip block).
    constexpr uint32 GOSSIP_MENU_FEED = 90001;
    constexpr uint32 NPC_TEXT_FEED = 90001;
    enum FeedGossipAction : uint32
    {
        FEED_ACTION_FEED = 1,
        FEED_ACTION_KEEP = 2,
    };

    // Gossip for the major choices (T-023), opened from the player: the tier list, then one tier's majors.
    constexpr uint32 GOSSIP_MENU_CHOICE = 90002;
    constexpr uint32 NPC_TEXT_CHOICE = 90002;
    constexpr uint32 NPC_TEXT_CHOICE_TIER = 90003;
    enum ChoiceGossipSender : uint32
    {
        CHOICE_SENDER_TIER   = 1,                     // action = tier: open that tier's majors
        CHOICE_SENDER_PICK   = 2,                     // action = tier * CHOICE_PICK_STRIDE + trait id
        CHOICE_SENDER_BACK   = 3,
        CHOICE_SENDER_CLOSE  = 4,
    };
    constexpr uint32 CHOICE_PICK_STRIDE = 1000000;

    // A Celestial shrine where chosen majors can be changed (world heartseed_shrine).
    struct ShrineInfo
    {
        uint32 Id = 0;
        uint32 MapId = 0;
        float X = 0.0f, Y = 0.0f, Z = 0.0f;
        float Radius = 0.0f;
        std::string Name;
    };

    // Story beats; stored as rows in heartseed_story_flag. Append only: the values are saved.
    enum StoryFlag : uint16
    {
        STORY_FLAG_FIRST_DUNGEON    = 1,
        STORY_FLAG_REACHED_PANDARIA = 2,
        STORY_FLAG_FIRST_WORLD_BOSS = 3,

        // 1000 and up: hints the character has seen. Stored with the story flags so they persist, but never
        // listed in Heartseed.Rank.AwakeningFlags, so they do not raise the cap.
        HINT_TWINK_WEAPON           = 1001,
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
        std::map<uint32, uint32> EssenceLastProc;     // essence id -> getMSTime() of its last proc
        uint32 EssenceProcCount = 0;                  // essence procs since login
        ObjectGuid MainHand;                          // main-hand weapon last seen, to find the one replaced
        ObjectGuid PendingFeed;                       // replaced weapon the feed prompt is asking about
        std::set<ObjectGuid> DeclinedFeeds;           // weapons the player chose to keep this session
        uint64 FirstNewItemGuid = 0;                  // items from this counter on were created after login
        uint32 FedWeapons = 0;                        // weapons fed in total (heartseed_fed_weapon)
        bool CapNoticeSent = false;                   // "it needs you to grow" is said once per session
    };

    // An essence type (world heartseed_essence_template): a minor trait by the weapon's origin. While it sits
    // in an open slot it rolls a proc on damage the character deals, like the core proc.
    struct EssenceInfo
    {
        uint32 Id = 0;
        std::string Name;
        uint32 ProcSpellId = 0;                       // names the damage and sets its school in the combat log
        float ProcChance = 0.0f;                      // percent per damaging hit
        float WeaponDamagePct = 0.0f;                 // share of the average weapon damage per swing
        std::string Description;
    };

    // Where an essence comes from when a weapon is fed (world heartseed_essence_source).
    struct EssenceSource
    {
        uint32 EssenceId = 0;
        uint32 ItemEntry = 0;                         // matches this item, or 0 to match by name
        std::string NamePattern;                      // lower case; matched anywhere in the item's name
        float Chance = 0.0f;                          // percent per weapon fed
    };

    enum class FeedResult
    {
        Ok,
        NoHeartseed,
        Disabled,                                     // Heartseed.Enabled = 0
        NotWeapon,                                    // not a weapon, or a fishing pole or miscellaneous one
        Excluded,                                     // quality worth no Growth (legendary, heirloom), quest item
        Twink,                                        // twink item: fed only when the player deletes it
        Refundable,                                   // can still go back to the vendor
        Equipped,                                     // only weapons in bags are fed
    };

    // Why a weapon is fed. Twink items are refused on replacement; deleting one feeds it.
    enum class FeedSource
    {
        Command,                                      // GM command or another system
        Replace,                                      // the player confirmed the prompt after an upgrade
        Delete,                                       // the player deleted it; the item is destroyed by the caller
    };

    // Where a weapon appearance was unlocked (characters character_weapon_appearance).
    enum AppearanceSource : uint8
    {
        APPEARANCE_FED      = 1,
        APPEARANCE_DELETED  = 2,
    };

    // What feeding a weapon gives; FeedPreview fills it without consuming anything.
    struct FeedOutcome
    {
        uint32 Growth = 0;
        std::vector<std::pair<uint32, float>> EssenceChances;   // essence id -> percent
        std::vector<uint32> Essences;                           // essences gained (Feed only)
        uint8 OldRank = 0;
        uint8 NewRank = 0;
    };

    enum class EssenceResult
    {
        Ok,
        NoHeartseed,
        UnknownEssence,
        NotCollected,
        SlotLocked,                                   // slot above what the character level opens
        WrongSlot,
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
    void LoadData();                       // world: heartseed_stage, _glow, _trait, _essence_template, _essence_source, twink_item
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

    // Growth earned in play (feeding now; quests, rares, events and dungeons later). Spends it on ranks up to the
    // cap and tells the player. Returns the ranks gained.
    uint8 AddGrowth(Player* player, uint32 amount);
    // Spends banked Growth after the cap rose (level, awakening). Returns the ranks gained.
    uint8 ApplyRankUps(Player* player);
    uint8 GetRankCap(Player const* player) const;
    uint32 GetAwakenings(Heartseed::State const& state) const;   // story flags that count toward the cap
    uint32 GetGrowthForNextRank(uint8 rank) const;
    float GetCapPerLevel() const { return _capPerLevel; }
    uint32 GetCapPerAwakening() const { return _capPerAwakening; }

    // Major choices (T-023). An open tier with no major can be chosen anywhere; changing a chosen major needs a
    // Celestial shrine unless Heartseed.Choice.ChangeAtShrineOnly is 0.
    void SendChoiceMenu(Player* player);
    void SendChoiceTierMenu(Player* player, uint8 tier);
    void OnChoiceGossipSelect(Player* player, uint32 sender, uint32 action);
    bool HasUnchosenOpenTier(Player* player) const;
    bool TierHasMajors(uint8 tier) const;               // a tier with no major rows offers no choice yet
    void ScheduleChoicePrompt(Player* player);          // on login, when a choice is waiting
    bool GetShrineAt(Player const* player, Heartseed::ShrineInfo& out) const;
    std::vector<Heartseed::ShrineInfo> GetShrines() const;
    bool IsChangeAtShrineOnly() const { return _choiceShrineOnly; }

    // Granting quest (T-025). Its completion grants the Heartseed, whichever way it is completed.
    void ScheduleGrantOffer(Player* player);        // on login, after Heartseed.Grant.OfferDelay
    bool CanOfferGrantQuest(Player* player) const;
    bool OfferGrantQuest(Player* player);           // shows the quest (or its reward window) now
    void OnQuestAdded(Player* player, Quest const* quest);
    void OnQuestRewarded(Player* player, Quest const* quest);
    uint32 GetGrantQuestId() const { return _grantQuestId; }

    // Story beats (T-026): simple hooks set the flags until later quests take over. Each sets its flag once,
    // tells the player and, for an awakening flag, ranks the Heartseed up to the new cap.
    void OnDungeonCompleted(Player* player);
    void OnMapChanged(Player* player);
    void OnKillCredit(Player* player, Creature* killed);
    // Sets a story flag the first time it is reached in play. False if already set or no Heartseed.
    bool ReachStoryBeat(Player* player, uint16 flag);
    std::vector<uint16> GetAwakeningFlags() const;

    // Feeding: a weapon in the character's bags is consumed for Growth and a chance at an essence by origin.
    Heartseed::FeedResult CanFeed(Player const* player, Item const* item, Heartseed::FeedSource source) const;
    void FeedPreview(ItemTemplate const* proto, uint32 itemLevel, Heartseed::FeedOutcome& out) const;
    Heartseed::FeedResult Feed(Player* player, Item* item, Heartseed::FeedOutcome& out, Heartseed::FeedSource source);
    // The main hand changed. When the weapon it replaced is now in the bags and has a lower item level
    // (Heartseed.Feed.OnReplace), the Heartseed asks, one update later so the swap has finished, whether to feed
    // it. Nothing is consumed until the player confirms.
    void OnMainHandChanged(Player* player, Item* newWeapon);
    void OfferReplacedFeed(Player* player, ObjectGuid oldWeapon);
    void OnFeedGossipSelect(Player* player, uint32 action);
    // The player deleted an item: a twink weapon is fed, every weapon's appearance is recorded.
    void OnItemDestroyedByPlayer(Player* player, Item* item);
    bool IsFeedOnReplace() const { return _feedOnReplace; }
    bool IsTwinkItem(uint32 entry) const;                // world twink_item
    uint32 GetTwinkItemCount() const;
    // Records a weapon appearance for the appearance collection (later) to read.
    static void RecordAppearance(Player* player, ItemTemplate const* proto, Heartseed::AppearanceSource source);

    // Essences. AddEssence puts a new essence into the first free open slot (Heartseed.Essence.AutoSlot).
    // SetEssenceSlot moves an essence to a slot (0 = inactive); whatever held that slot goes inactive. force
    // skips the level check (GM commands); the shrine gossip (T-023) calls it without.
    Heartseed::EssenceResult AddEssence(Player* player, uint32 essenceId, uint32 count);
    Heartseed::EssenceResult RemoveEssence(Player* player, uint32 essenceId);
    Heartseed::EssenceResult SetEssenceSlot(Player* player, uint32 essenceId, uint8 slot, bool force);
    std::vector<Heartseed::EssenceInfo> GetEssences() const;
    bool GetEssence(uint32 essenceId, Heartseed::EssenceInfo& out) const;
    std::vector<Heartseed::EssenceSource> GetEssenceSources() const;

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
    // essenceId 0 is the core proc.
    void CastProc(Player* player, Unit* victim, uint32 essenceId);
    uint32 GetProcDamage(Player* player, float weaponDamagePct) const;
    uint32 GetProcDamage(Player* player) const { return GetProcDamage(player, _procWeaponDamagePct); }
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
    float GetFeedQualityPct(uint32 quality) const;
    void SendRankUp(Player* player, uint8 oldRank, uint8 newRank);

    bool _enabled = true;
    float _coreWeaponDamagePct = 0.0f;
    uint32 _procSpellId = 0;
    float _procChance = 0.0f;
    float _procWeaponDamagePct = 0.0f;
    uint32 _procCooldown = 0;
    float _capPerLevel = 1.0f;
    uint32 _capPerAwakening = 0;
    std::vector<uint16> _awakeningFlags;
    bool _storyHooks = true;
    std::vector<uint32> _pandariaMaps;
    std::set<uint32> _worldBossEntries;
    uint32 _awakeningSound = 0;
    uint32 _grantQuestId = 0;
    bool _grantOfferOnLogin = true;
    uint32 _grantOfferDelay = 0;
    bool _choicePromptOnLogin = true;
    uint32 _choicePromptDelay = 0;
    bool _choiceShrineOnly = true;
    std::vector<Heartseed::ShrineInfo> _shrines;
    uint32 _rankGrowthBase = 0;
    uint32 _rankGrowthPerRank = 0;
    bool _feedOnReplace = true;
    uint32 _feedGrowthBase = 0;
    float _feedGrowthPerItemLevel = 0.0f;
    std::vector<float> _feedQualityPct;                 // by item quality, 0 = never fed
    uint32 _essenceProcCooldown = 0;
    bool _essenceAutoSlot = true;
    std::vector<Heartseed::StageInfo> _stages;
    std::vector<uint8> _stageMinLevels;
    std::map<std::pair<uint8, uint8>, uint16> _glow;    // (stage, path) -> enchant id
    std::map<uint32, Heartseed::TraitInfo> _traits;     // trait id -> trait
    std::map<uint32, Heartseed::EssenceInfo> _essences; // essence id -> essence
    std::vector<Heartseed::EssenceSource> _essenceSources;
    std::set<uint32> _twinkItems;                       // item entries flagged as twink gear

    mutable std::mutex _lock;                           // players save from map threads
    std::unordered_map<uint32, Heartseed::State> _states;   // guid counter -> state
};

#define sHeartseedMgr HeartseedMgr::instance()

#endif
