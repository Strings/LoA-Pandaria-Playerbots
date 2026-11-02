/*
 * Player telemetry (T-004): the WoW adapter around TelemetryCore. Builds context snapshots from server
 * state, keeps one session per tracked character and writes JSON lines under Telemetry.Directory.
 * Schema: README.md beside this file.
 */

#ifndef TELEMETRY_MGR_H
#define TELEMETRY_MGR_H

#include "Define.h"
#include "ObjectGuid.h"
#include "TelemetryCore.h"

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

class Map;
class Player;
class Spell;
class Unit;
class WorldObject;

class TelemetryMgr
{
public:
    static TelemetryMgr* instance();

    void LoadConfig();
    bool Enabled() const { return _enabled; }
    uint32 DiscardWindowMinutes() const { return uint32(_discardWindowMs / 60000); }

    void OnLogin(Player* player);
    void OnLogout(Player* player);
    void OnPlayerUpdate(Player* player);
    void OnWorldUpdate(uint32 diff);
    void OnShutdown();

    void OnCastStart(Player* player, Spell const* spell);
    void OnCastDone(Player* player, Spell const* spell);
    void OnCastCancel(Player* player, Spell const* spell);
    void OnCommand(Player* player, char const* text);
    void OnLootOpen(Player* player, WorldObject const* source);
    void OnLootRoll(Player* player, uint32 itemId, uint8 choice);
    void OnObjective(Map* map, char const* kind, uint32 id, uint32 value);
    void OnChat(Player* player, uint32 type, uint32 lang, std::string const& msg);
    void OnEmote(Player* player, uint32 textEmote);
    void OnDamage(Unit* attacker, Unit* victim, uint32 damage);
    void OnKill(Player* killer, Unit* victim);
    void OnKilledBy(Unit* killer, Player* victim);
    void OnZone(Player* player, uint32 zone, uint32 area);

    // Commands; each returns false when the character has no session.
    bool Describe(Player* player, std::string& out);
    bool SetManualPause(Player* player, bool paused);
    bool Discard(Player* player, uint32 minutes, size_t& dropped, uint32& appliedMinutes);
    bool Tag(Player* player, std::string const& intent);
    bool Snapshot(Player* player, std::string& out);
    bool Flush(Player* player, size_t& written);
    // Logs a playerbot as a scripted_bot actor (sparring, the bot-tells discriminator); bots are never
    // logged otherwise.
    bool Track(Player* bot, bool on);

private:
    struct Session
    {
        std::string Id;
        ObjectGuid Guid;
        uint32 AccountId = 0;
        bool Bot = false;
        uint64 StartMs = 0;
        std::filesystem::path File;
        Telemetry::RowBuffer Rows;
        uint64 RowsWritten = 0;
        Telemetry::PauseState Pause;
        uint64 PausedSinceMs = 0;
        Telemetry::EpisodeTracker Episode;
        std::string Intent;
        Telemetry::FightTracker Fight;
        bool Pull = false;

        bool HasMove = false;
        Telemetry::MoveSample LastMove;
        uint64 LastMoveMs = 0;
        uint64 LastInputMs = 0;
        uint64 LastFacingChangeMs = 0;
        uint64 LastHostileCastMs = 0;
        uint64 LastGatherMs = 0;

        ObjectGuid Target;
        bool InCombat = false;
        bool Mounted = false;
        bool Taxi = false;
        bool Dead = false;
        bool Ghost = false;
        uint64 DeathMs = 0;
        std::string PendingKiller;      // JSON from the kill hooks, logged with the death edge
    };

    using Lines = std::vector<std::pair<std::filesystem::path, std::vector<std::string>>>;

    bool Wanted(Player* player) const;
    Session* Find(Player* player);
    void StartSession(Player* player, bool bot);
    Telemetry::JsonObject Row(Session const& s, char const* type, uint64 now) const;
    // Paused sessions keep only gap edges and session start and end (`always`).
    void Push(Session& s, uint64 now, Telemetry::JsonObject const& row, bool always = false);
    // Adds the context snapshot; decision events are the rows training learns choices from.
    void Decision(Session& s, Player* player, uint64 now, Telemetry::JsonObject row, Unit* target);
    void SetPause(Session& s, uint32 reason, bool on, uint64 now);
    void Input(Session& s, uint64 now);
    void UpdateEpisode(Session& s, Player* player, uint64 now);

    std::string Context(Session const& s, Player* player, Unit* target, uint64 now, bool decision);
    std::string SelfJson(Session const& s, Player* player, uint64 now);
    std::string TargetJson(Player* player, Unit* target, bool decision);
    std::string EnvJson(Player* player, Unit* target);
    std::string PlaceJson(Session const& s, Player* player, uint64 now);
    int32 InterruptCooldown(Player* player) const;

    static void Write(Lines const& lines);

    std::atomic<bool> _enabled{ false };
    bool _chat = false;
    uint32 _moveIntervalMs = 200;
    uint64 _idleTimeoutMs = 300000;
    uint64 _idleEpisodeMs = 30000;
    uint64 _discardWindowMs = 900000;
    uint32 _flushIntervalMs = 5000;
    uint64 _episodeHoldMs = 3000;
    float _packRadius = 60.0f;
    std::vector<uint32> _interruptSpells;
    std::filesystem::path _directory;

    std::atomic<uint32> _trackedBots{ 0 };
    uint32 _flushTimer = 0;
    std::mutex _lock;
    std::unordered_map<uint64, Session> _sessions;
};

#define sTelemetryMgr TelemetryMgr::instance()

#endif
