/*
 * Heartseed state per account and per character (characters DB) and stage/glow data (world DB).
 * One seed per account (T-049): the essence collection and the weapons fed are shared by every character of the
 * account; loadouts, essence slotting, story flags, Growth XP and stat points belong to each character (T-083).
 * See README.md in this folder and docs/DESIGN.md, "The Heartseed".
 */

#ifndef HEARTSEED_MGR_H
#define HEARTSEED_MGR_H

#include "Define.h"
#include "HeartseedRules.h"
#include "ObjectGuid.h"

#include <cstddef>
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
    // Server-side passive (spell_dbc) whose crit procs (spell_proc_event) feed the path meter.
    constexpr uint32 SPELL_METER_PASSIVE = 310009;
    // Client spells the Gifts were before they got their own clones (T-086), removed from players on sync.
    constexpr uint32 RETIRED_GIFT_SPELLS[] = { 123904, 148009, 116098, 117960, 115008, 119381 };
    // Server-side passives (spell_dbc) for "main stat" Growth points, picked by the character's main stat.
    constexpr uint32 SPELL_GROWTH_STRENGTH = 310011;
    constexpr uint32 SPELL_GROWTH_AGILITY = 310012;
    constexpr uint32 SPELL_GROWTH_INTELLECT = 310013;

    // A stat Growth points can go to (world heartseed_growth_stat).
    struct GrowthStat
    {
        uint8 Id = 0;
        std::string Name;
        bool MainStat = false;                        // spell picked by the character's main stat, not SpellId
        uint32 SpellId = 0;                           // passive aura; effect 0 = StatPointAmount
        int32 MaxPct = 0;                             // the ceiling the bonus approaches
        uint32 HalfPoints = 0;                        // points that give half of MaxPct
    };

    enum class GrowthResult
    {
        Ok,
        NoHeartseed,
        UnknownStat,
        NoPoints,                                     // nothing unspent (spend) or nothing spent (reset)
        NotAtShrine,
    };

    // What casting the Gift spent from the path meter; the Gift's effects read it while they land.
    struct GiftSpend
    {
        uint8 Stacks = 0;
        int32 BonusPctPerStack = 0;
        uint32 RecentDamage = 0;                      // damage taken in the meter window (Niuzao's Retort)
    };

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

        // 1000 and up: hints the character has seen, stored with the story flags so they persist.
        HINT_TWINK_WEAPON           = 1001,
        HINT_GROWTH_POINTS          = 1002,
    };

    struct State
    {
        // Shared by the account (account_heartseed*): every loaded character of the account holds the same
        // values, and a change to one is copied to the others.
        uint32 AccountId = 0;
        uint32 GrantedTime = 0;
        uint32 FedWeapons = 0;                        // weapons fed in total (account_heartseed_fed_weapon)
        // essence id -> (times collected: account, active slot or 0: this character)
        std::map<uint32, std::pair<uint32, uint8>> Essences;

        // This character's own (heartseed_loadout, heartseed_essence slots, heartseed_story_flag).
        // spec id -> trait chosen at each major tier (0 = not chosen)
        std::map<uint16, std::array<uint32, MaxMajorTiers>> Loadouts;
        // story flag -> unix time it was set
        std::map<uint16, uint32> StoryFlags;
        // character_heartseed_growth*: Growth level, XP toward the next one, stat id -> points spent.
        uint32 GrowthLevel = 0;
        uint32 GrowthXp = 0;
        std::map<uint8, uint32> GrowthPoints;
        bool Dirty = false;

        // Runtime only, never saved: what RefreshTraits last applied to the character.
        std::map<uint32, int32> AppliedAuras;         // spell id -> effect 0 amount, core aura included
        std::set<uint32> TaughtGifts;                 // Gift spells RefreshTraits taught the character
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
        uint32 GiftTraitId = 0;                       // the taught Gift's trait, whose path meter is built
        Heartseed::MeterState Meter;
        std::vector<std::pair<uint32, uint32>> RecentHits;   // (getMSTime(), damage) taken, for the meter window
        GiftSpend LastSpend;
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
        Excluded,                                     // quality not in Heartseed.Feed.Qualities, quest item
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
        std::vector<std::pair<uint32, float>> EssenceChances;   // essence id -> percent
        std::vector<uint32> Essences;                           // essences gained (Feed only)
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
        uint8 MinLevel = 1;                           // minors: character level that fills it in (column min_rank)
        uint32 SpellId = 0;                           // passive aura applied while the trait is active; 0 = none
        uint32 GiftSpellId = 0;                       // Celestial Gift: active spell taught while the trait is active
        uint32 GiftCooldown = 0;                      // Gift cooldown in ms, replacing the spell's own; 0 keeps it
        uint32 GiftDuration = 0;                      // Gift summon lifetime in ms, replacing the spell's; 0 keeps it
        // Gift power in %: of attack or spell power (Serpent's Breath tick), of max health (Crane's Plume heal), of
        // the damage taken in the meter window (Niuzao's Retort per enemy). Kept relative so low levels aren't overtuned.
        int32 GiftBaseAmount = 0;
        float GiftAmountPerItemLevel = 0.0f;          // flat power added per main-hand item level (0 in shipped data)
        std::vector<GiftRider> GiftRiders;            // heartseed_gift_rider, ordered by stage
        int32 BaseAmount = 0;                        // effect 0 amount at item level 0
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
    // Reload the related world tables atomically; failures leave the live cache intact.
    bool LoadData(std::size_t* loadedRows = nullptr);
    void LoadAccounts();                   // characters: which accounts have a Heartseed, so bots skip the queries
    bool IsEnabled() const { return _enabled; }

    // Players log in with their account's Heartseed; a playerbot only when its account has one (the player's
    // own alts), so random bots cost no queries.
    bool ShouldLoad(Player const* player) const;
    void LoadPlayer(Player* player);       // characters: account_heartseed* of the account, heartseed_* of the character
    void SavePlayer(Player* player);       // writes state if changed
    void UnloadPlayer(Player* player);

    bool HasHeartseed(Player const* player) const;
    // Copy of the state, so callers never hold a pointer into the map other threads may change.
    bool GetState(Player const* player, Heartseed::State& out) const;
    bool Grant(Player* player);            // false if it already has one; grants the whole account
    bool Revoke(Player* player);           // deletes the account's Heartseed and every heartseed_* row of its characters

    // State changes for GM commands and later systems (feeding, story hooks). Each saves at once and
    // returns false when the character has no Heartseed.
    bool SetStoryFlag(Player* player, uint16 flag, bool set);

    // Growth XP (T-083), this character's own: battleground wins and dungeon and raid bosses near its level. Only
    // the character being played earns, and with Heartseed.Growth.TwinksEarn 0 not one with XP gain turned off.
    // Levels up and tells the player; returns the levels gained.
    uint32 AddGrowthXp(Player* player, uint32 amount);
    // GM and checkpoints: sets the level and XP; points spent past the new level are refunded, highest stat first.
    bool SetGrowthLevel(Player* player, uint32 level, uint32 xp);
    uint32 GetGrowthXpForLevel(uint32 level) const;
    void OnBattlegroundEnd(Player* player, bool won);
    // Stat points: one per Growth level, spent by the player (.seed spend, the addon). Reset is free but only at a
    // Celestial shrine unless Heartseed.Growth.ResetAtShrineOnly is 0; anywhere skips that (GM).
    Heartseed::GrowthResult SpendGrowthPoints(Player* player, uint8 statId, uint32 count);
    Heartseed::GrowthResult ResetGrowthPoints(Player* player, bool anywhere);
    std::vector<Heartseed::GrowthStat> GetGrowthStats() const;
    bool GetGrowthStat(uint8 statId, Heartseed::GrowthStat& out) const;
    // The aura a stat's points go to for this character (main stat: by specialization). Takes no lock.
    static uint32 GetGrowthStatSpell(Player const* player, Heartseed::GrowthStat const& stat);

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
    uint32 GetGrantOfferDelay() const { return _grantOfferDelay; }
    uint32 GetChoicePromptDelay() const { return _choicePromptDelay; }

    // Story beats (T-026): simple hooks set the flags until later quests take over. Each sets its flag once and
    // tells the player. Kill credit also pays Growth XP for dungeon and raid bosses.
    void OnDungeonCompleted(Player* player);
    void OnMapChanged(Player* player);
    void OnKillCredit(Player* player, Creature* killed);
    // Sets a story flag the first time it is reached in play. False if already set or no Heartseed.
    bool ReachStoryBeat(Player* player, uint16 flag);

    // Feeding: a weapon in the character's bags is consumed for its appearance and a chance at an essence by origin.
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
    // Spec loadouts (T-017) keep majors per loadout and write them back for the spec before switching to it.
    // No shrine or level check: the choices were made before. False when the character has no Heartseed.
    bool SetSpecMajors(Player* player, uint16 specId, std::array<uint32, Heartseed::MaxMajorTiers> const& majors);

    std::vector<Heartseed::TraitInfo> GetTraits() const;
    bool GetTrait(uint32 traitId, Heartseed::TraitInfo& out) const;

    // Brings the character's trait auras and core weapon bonus in line with its state, spec, level and
    // main-hand weapon. Cheap when nothing changed.
    void RefreshTraits(Player* player);
    // The active Celestial Gift trait that taught spellId to the character; false when none did. Spell scripts
    // use it for the Gift cooldown and power. spellId 0 finds whichever Gift the character was taught.
    bool GetActiveGift(Player const* player, uint32 spellId, Heartseed::TraitInfo& out) const;
    // Serpent's Breath tick: GiftBaseAmount % of the higher of attack power and spell power.
    int32 GetGiftAmount(Player const* player, Heartseed::TraitInfo const& gift) const;
    // The Gift's riders the character's stage has reached.
    std::vector<Heartseed::GiftRider> GetGiftRiders(Player const* player, Heartseed::TraitInfo const& gift) const;
    // Rider of this kind from spellId's active Gift; false when the character has none.
    bool GetGiftRider(Player const* player, uint32 spellId, Heartseed::GiftRiderKind kind, Heartseed::GiftRider& out) const;
    // Cuts the Gift's cooldown by ms (0 = reset), never past ready.
    void CutGiftCooldown(Player* player, uint32 spellId, int32 ms) const;

    // Path meters (heartseed_meter). Play events build the meter of the taught Gift's path; amount is the health
    // gained or the damage taken for Healing and BigHit. spellId is the spell behind the event, so the Gift's own
    // casts and crits don't count.
    void OnMeterEvent(Player* player, Heartseed::MeterSource source, uint32 amount, uint32 spellId = 0);
    // A gated Gift (Niuzao's Retort) casts only while lit; every other spell is always allowed.
    bool CanCastGift(Player const* player, uint32 spellId) const;
    // Casting the Gift spends the meter; GetLastSpend returns it while the Gift's effects land.
    Heartseed::GiftSpend SpendGiftMeter(Player* player, uint32 spellId);
    Heartseed::GiftSpend GetLastSpend(Player const* player) const;
    bool GetMeter(Player const* player, Heartseed::MeterRule& rule, Heartseed::MeterState& meter) const;
    // GM: sets the stacks (capped) or lights a gated Gift. False when the character has no Gift with a meter.
    bool ForceMeter(Player* player, uint8 stacks, bool lit);
    // Tells the addon the meter: "P" <stacks> <max> <lit ms left> <decay ms> <Gift spell>.
    void SendMeter(Player* player) const;
    float GetPlumeRadius() const { return _plumeRadius; }
    uint32 GetPlumeTargets() const { return _plumeTargets; }
    float GetRetortRadius() const { return _retortRadius; }
    float GetBreathRange() const { return _breathRange; }
    // Traits plus glow; called after any state change.
    void Refresh(Player* player);

    // Core passive proc: rolled on damage the character deals, dealt one update later.
    void OnDamageDealt(Player* player, Unit* victim);
    // essenceId 0 is the core proc.
    void CastProc(Player* player, Unit* victim, uint32 essenceId);
    uint32 GetProcDamage(Player* player, float weaponDamagePct) const;
    uint32 GetProcDamage(Player* player) const { return GetProcDamage(player, _procWeaponDamagePct); }
    uint32 GetProcSpellId() const { return _procSpellId; }
    uint32 GetXuenLightningVisual() const { return _xuenLightningVisual; }
    float GetXuenCatchUpDistance() const { return _xuenCatchUpDistance; }
    uint32 GetXuenFollowInterval() const { return _xuenFollowInterval; }

    uint8 GetStage(Player const* player) const;
    std::string GetStageName(uint8 stage) const;
    Heartseed::Path GetDominantPath(Player const* player) const;
    uint16 GetGlowEnchant(Player const* player) const;
    void RefreshGlow(Player* player);      // re-sends the main hand's visible item field

private:
    HeartseedMgr() = default;

    bool ModifyState(Player* player, std::function<void(Heartseed::State&)> const& change);
    // Callers hold _lock.
    // Copies the account's shared fields from one character's state to the account's other loaded characters.
    void ShareAccountState(uint32 guid, Heartseed::State const& from);
    std::array<uint8, Heartseed::MaxPaths> MajorsPerPath(Heartseed::State const& state, uint16 specId) const;
    static void RemoveAuras(Player* player, std::map<uint32, int32> const& auras);
    // Teaches the wanted Gift spells and unlearns the others, unless the character has them from its own talents.
    void SyncGifts(Player* player, std::set<uint32> const& wanted, bool announce);
    bool CanFeedQuality(uint32 quality) const;

    bool _enabled = true;
    uint32 _xuenLightningVisual = 0;
    float _xuenCatchUpDistance = 0.0f;
    uint32 _xuenFollowInterval = 1000;
    float _plumeRadius = 30.0f;
    uint32 _plumeTargets = 3;
    float _retortRadius = 8.0f;
    float _breathRange = 12.0f;
    std::map<uint8, Heartseed::MeterRule> _meters;      // path -> meter (heartseed_meter)
    float _coreWeaponDamagePct = 0.0f;
    uint32 _procSpellId = 0;
    float _procChance = 0.0f;
    float _procWeaponDamagePct = 0.0f;
    uint32 _procCooldown = 0;
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
    uint32 _growthXpBase = 0;
    uint32 _growthXpPerLevel = 0;
    uint32 _growthBattlegroundWin = 0;
    uint32 _growthDungeonBoss = 0;
    uint32 _growthRaidBoss = 0;
    uint32 _growthQuest = 0;
    uint32 _growthRare = 0;
    uint8 _growthLevelRange = 0;
    bool _growthTwinksEarn = false;
    bool _growthResetAtShrineOnly = true;
    std::map<uint8, Heartseed::GrowthStat> _growthStats; // stat id -> stat (heartseed_growth_stat)
    bool _feedOnReplace = true;
    std::set<uint32> _feedQualities;                    // item qualities that can be fed
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
    std::set<uint32> _seedAccounts;                     // accounts with a Heartseed (account_heartseed)
};

#define sHeartseedMgr HeartseedMgr::instance()

#endif
