/*
 * Game-agnostic telemetry core (T-004): JSON rows, pause reasons, the discard window, activity episodes,
 * movement sampling and the small geometry the context snapshot needs. No server dependencies, so it can
 * ship with the open bot brain and be unit-tested directly.
 */

#ifndef TELEMETRY_CORE_H
#define TELEMETRY_CORE_H

#include <cstdint>
#include <deque>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace Telemetry
{
    // Bump when a field changes meaning or is removed; added fields keep the version.
    constexpr uint32_t SchemaVersion = 1;

    std::string JsonEscape(std::string_view text);
    // Up to `decimals` places with trailing zeros trimmed; NaN and infinities become null.
    std::string JsonNumber(double value, int decimals);

    // Builds one JSON object a field at a time. Keys are trusted literals; string values are escaped.
    class JsonObject
    {
    public:
        template <typename T> requires std::is_integral_v<T>
        JsonObject& Add(std::string_view key, T value)
        {
            Key(key);
            if constexpr (std::is_same_v<T, bool>)
                _text += value ? "true" : "false";
            else if constexpr (std::is_signed_v<T>)
                _text += std::to_string(static_cast<int64_t>(value));
            else
                _text += std::to_string(static_cast<uint64_t>(value));
            return *this;
        }
        JsonObject& Add(std::string_view key, double value, int decimals = 2);
        JsonObject& Add(std::string_view key, std::string_view value);
        JsonObject& Add(std::string_view key, char const* value) { return Add(key, std::string_view(value)); }
        JsonObject& Add(std::string_view key, std::string const& value) { return Add(key, std::string_view(value)); }
        // `json` must already be a valid JSON value (object, array, number).
        JsonObject& AddRaw(std::string_view key, std::string_view json);
        JsonObject& AddNull(std::string_view key);

        bool Empty() const { return _text.size() == 1; }
        std::string Str() const { return _text + "}"; }

    private:
        void Key(std::string_view key);
        std::string _text = "{";
    };

    class JsonArray
    {
    public:
        template <typename T> requires std::is_integral_v<T>
        JsonArray& Add(T value)
        {
            Sep();
            if constexpr (std::is_signed_v<T>)
                _text += std::to_string(static_cast<int64_t>(value));
            else
                _text += std::to_string(static_cast<uint64_t>(value));
            return *this;
        }
        JsonArray& Add(double value, int decimals = 2);
        JsonArray& AddRaw(std::string_view json);

        size_t Size() const { return _count; }
        std::string Str() const { return _text + "]"; }

    private:
        void Sep();
        std::string _text = "[";
        size_t _count = 0;
    };

    // Why logging is paused. Several can hold at once; logging runs only when none do.
    enum PauseReason : uint32_t
    {
        PAUSE_MANUAL     = 0x01,    // .telemetry pause
        PAUSE_GM_MODE    = 0x02,
        PAUSE_GM_COMMAND = 0x04,    // until the next move or cast
        PAUSE_AFK        = 0x08,
        PAUSE_IDLE       = 0x10,    // no input for the idle timeout; any input clears it
    };

    std::string PauseReasonNames(uint32_t reasons);

    class PauseState
    {
    public:
        // Returns true when logging flipped between running and paused, so the caller logs a gap edge.
        bool Set(uint32_t reason, bool on);
        // A move or cast clears the pauses that only last until the player acts again.
        bool OnInput() { return Set(PAUSE_GM_COMMAND | PAUSE_IDLE, false); }
        bool Paused() const { return _reasons != 0; }
        uint32_t Reasons() const { return _reasons; }

    private:
        uint32_t _reasons = 0;
    };

    // Rows stay here for the discard window before they are final and written, so `.telemetry discard`
    // can still drop them.
    class RowBuffer
    {
    public:
        void Push(uint64_t timeMs, std::string line);
        // Rows at least `windowMs` old, oldest first.
        std::vector<std::string> TakeFinal(uint64_t nowMs, uint64_t windowMs);
        std::vector<std::string> TakeAll();
        // Drops rows from the last `spanMs`; returns how many went.
        size_t Discard(uint64_t nowMs, uint64_t spanMs);
        size_t Size() const { return _rows.size(); }

    private:
        struct Row
        {
            uint64_t TimeMs;
            std::string Line;
        };
        std::deque<Row> _rows;
    };

    // What the character is doing, from server state. Context (world, dungeon, PvP...) is tracked beside it.
    enum class Activity : uint8_t
    {
        Idle,
        Travel,
        Questing,       // out in the world, acting, and nothing more specific applies
        Gathering,
        Vendor,         // vendor, bank, trainer, mailbox and similar services
        Fight,
    };

    char const* ActivityName(Activity activity);

    struct ActivityInputs
    {
        bool InCombat = false;
        bool Gathering = false;
        bool AtService = false;
        bool Traveling = false;     // mounted, on a flight path, or covering ground out of combat
        bool Idle = false;          // no input for the idle-episode threshold
    };

    Activity ClassifyActivity(ActivityInputs const& inputs);

    // Splits play into episodes. A new activity must hold for `holdMs` before the episode switches, so a
    // one-tick flicker never splits one; fights and context changes switch at once.
    class EpisodeTracker
    {
    public:
        // Returns true when a new episode started.
        bool Update(Activity activity, uint8_t context, uint64_t nowMs, uint64_t holdMs);
        // The next update starts a new episode: episodes never stitch across a logging gap.
        void Break() { _broken = true; }
        uint32_t Id() const { return _id; }
        Activity Current() const { return _current; }
        uint8_t Context() const { return _context; }
        uint64_t StartMs() const { return _startMs; }

    private:
        uint32_t _id = 0;
        Activity _current = Activity::Idle;
        uint8_t _context = 0;
        uint64_t _startMs = 0;
        bool _broken = false;
        bool _hasCandidate = false;
        Activity _candidate = Activity::Idle;
        uint64_t _candidateSinceMs = 0;
    };

    struct MoveSample
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Z = 0.0f;
        float O = 0.0f;
        uint32_t Flags = 0;
        uint32_t Flags2 = 0;
    };

    bool MoveChanged(MoveSample const& last, MoveSample const& now);
    // A flag change (start strafing, jump, stop) logs at once; other changes once per interval. Nothing
    // logs while standing still with the same facing.
    bool MoveSampleDue(MoveSample const& last, uint64_t lastMs, MoveSample const& now, uint64_t nowMs, uint32_t intervalMs);

    // Wraps to [-pi, pi].
    float NormalizeAngle(float angle);
    // Angle from a facing to a point: 0 straight ahead, positive to the left (counterclockwise).
    float RelativeAngle(float fromX, float fromY, float facing, float toX, float toY);

    struct Point
    {
        float X = 0.0f;
        float Y = 0.0f;
        float Facing = 0.0f;
    };

    // Where the friendly pack is relative to the character: distance and angle to the allies' centroid and
    // their mean heading relative to the character's facing.
    struct PackSummary
    {
        uint32_t Count = 0;
        float Distance = 0.0f;
        float Angle = 0.0f;
        float Heading = 0.0f;
    };

    PackSummary SummarizePack(Point const& self, std::vector<Point> const& allies);

    // How far through its cast a spell was (0 to 1) given its full time and what is left.
    float CastProgress(int32_t totalMs, int32_t remainingMs);

    struct FightSummary
    {
        uint64_t DurationMs = 0;
        uint64_t DamageTaken = 0;
        uint64_t DamageDone = 0;
        uint32_t Kills = 0;
    };

    // Damage and kills per fight, and time since the last fight ended.
    class FightTracker
    {
    public:
        void Start(uint64_t nowMs);
        FightSummary End(uint64_t nowMs);
        void AddTaken(uint64_t amount) { if (_active) _current.DamageTaken += amount; }
        void AddDone(uint64_t amount) { if (_active) _current.DamageDone += amount; }
        void AddKill() { if (_active) ++_current.Kills; }
        bool Active() const { return _active; }
        // False before the first fight ends and while one is running.
        bool SinceEnd(uint64_t nowMs, uint64_t& sinceMs) const;

    private:
        bool _active = false;
        bool _ended = false;
        uint64_t _startMs = 0;
        uint64_t _endMs = 0;
        FightSummary _current;
    };
}

#endif
