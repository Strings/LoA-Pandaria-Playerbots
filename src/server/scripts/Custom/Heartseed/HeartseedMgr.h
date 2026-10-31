/*
 * Heartseed state per character (characters DB) and stage/glow data (world DB).
 * See README.md in this folder and docs/DESIGN.md, "The Heartseed".
 */

#ifndef HEARTSEED_MGR_H
#define HEARTSEED_MGR_H

#include "Define.h"
#include "HeartseedRules.h"

#include <map>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class Player;

namespace Heartseed
{
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
    void LoadData();                       // world: heartseed_stage, heartseed_glow
    bool IsEnabled() const { return _enabled; }

    void LoadPlayer(Player* player);       // characters: all heartseed_* rows of one character
    void SavePlayer(Player* player);       // writes state if changed
    void UnloadPlayer(Player* player);

    bool HasHeartseed(Player const* player) const;
    // Copy of the state, so callers never hold a pointer into the map other threads may change.
    bool GetState(Player const* player, Heartseed::State& out) const;
    bool Grant(Player* player);            // false if it already has one
    bool Revoke(Player* player);           // deletes every heartseed_* row of the character

    uint8 GetStage(Player const* player) const;
    std::string GetStageName(uint8 stage) const;
    Heartseed::Path GetDominantPath(Player const* player) const;
    uint16 GetGlowEnchant(Player const* player) const;
    void RefreshGlow(Player* player);      // re-sends the main hand's visible item field

private:
    HeartseedMgr() = default;

    bool _enabled = true;
    std::vector<Heartseed::StageInfo> _stages;
    std::vector<uint8> _stageMinLevels;
    std::map<std::pair<uint8, uint8>, uint16> _glow;    // (stage, path) -> enchant id

    mutable std::mutex _lock;                           // players save from map threads
    std::unordered_map<uint32, Heartseed::State> _states;   // guid counter -> state
};

#define sHeartseedMgr HeartseedMgr::instance()

#endif
