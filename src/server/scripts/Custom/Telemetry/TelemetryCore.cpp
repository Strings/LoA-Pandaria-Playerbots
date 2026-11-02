#include "TelemetryCore.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace Telemetry
{
    namespace
    {
        constexpr float Pi = 3.14159265358979f;
    }

    std::string JsonEscape(std::string_view text)
    {
        std::string out;
        out.reserve(text.size() + 2);
        for (char c : text)
        {
            switch (c)
            {
                case '"': out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n"; break;
                case '\r': out += "\\r"; break;
                case '\t': out += "\\t"; break;
                default:
                    if (static_cast<unsigned char>(c) < 0x20)
                    {
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned>(static_cast<unsigned char>(c)));
                        out += buf;
                    }
                    else
                        out += c;   // UTF-8 bytes pass through
                    break;
            }
        }
        return out;
    }

    std::string JsonNumber(double value, int decimals)
    {
        if (!std::isfinite(value))
            return "null";
        char buf[64];
        std::snprintf(buf, sizeof(buf), "%.*f", std::clamp(decimals, 0, 9), value);
        std::string text = buf;
        if (text.find('.') != std::string::npos)
        {
            text.erase(text.find_last_not_of('0') + 1);
            if (text.back() == '.')
                text.pop_back();
        }
        if (text == "-0")
            text = "0";
        return text;
    }

    void JsonObject::Key(std::string_view key)
    {
        if (_text.size() > 1)
            _text += ',';
        _text += '"';
        _text += key;
        _text += "\":";
    }

    JsonObject& JsonObject::Add(std::string_view key, double value, int decimals)
    {
        Key(key);
        _text += JsonNumber(value, decimals);
        return *this;
    }

    JsonObject& JsonObject::Add(std::string_view key, std::string_view value)
    {
        Key(key);
        _text += '"';
        _text += JsonEscape(value);
        _text += '"';
        return *this;
    }

    JsonObject& JsonObject::AddRaw(std::string_view key, std::string_view json)
    {
        Key(key);
        _text += json;
        return *this;
    }

    JsonObject& JsonObject::AddNull(std::string_view key)
    {
        Key(key);
        _text += "null";
        return *this;
    }

    void JsonArray::Sep()
    {
        if (_count++)
            _text += ',';
    }

    JsonArray& JsonArray::Add(double value, int decimals)
    {
        Sep();
        _text += JsonNumber(value, decimals);
        return *this;
    }

    JsonArray& JsonArray::AddRaw(std::string_view json)
    {
        Sep();
        _text += json;
        return *this;
    }

    std::string PauseReasonNames(uint32_t reasons)
    {
        static struct { uint32_t Bit; char const* Name; } const names[] = {
            { PAUSE_MANUAL, "manual" }, { PAUSE_GM_MODE, "gm_mode" }, { PAUSE_GM_COMMAND, "gm_command" },
            { PAUSE_AFK, "afk" }, { PAUSE_IDLE, "idle" } };
        std::string out;
        for (auto const& entry : names)
            if (reasons & entry.Bit)
            {
                if (!out.empty())
                    out += ',';
                out += entry.Name;
            }
        return out;
    }

    bool PauseState::Set(uint32_t reason, bool on)
    {
        bool wasPaused = Paused();
        if (on)
            _reasons |= reason;
        else
            _reasons &= ~reason;
        return wasPaused != Paused();
    }

    void RowBuffer::Push(uint64_t timeMs, std::string line)
    {
        _rows.push_back({ timeMs, std::move(line) });
    }

    std::vector<std::string> RowBuffer::TakeFinal(uint64_t nowMs, uint64_t windowMs)
    {
        std::vector<std::string> out;
        // Rows arrive in time order, so the final ones are a prefix.
        while (!_rows.empty() && _rows.front().TimeMs + windowMs <= nowMs)
        {
            out.push_back(std::move(_rows.front().Line));
            _rows.pop_front();
        }
        return out;
    }

    std::vector<std::string> RowBuffer::TakeAll()
    {
        std::vector<std::string> out;
        out.reserve(_rows.size());
        for (Row& row : _rows)
            out.push_back(std::move(row.Line));
        _rows.clear();
        return out;
    }

    size_t RowBuffer::Discard(uint64_t nowMs, uint64_t spanMs)
    {
        uint64_t from = nowMs > spanMs ? nowMs - spanMs : 0;
        size_t dropped = 0;
        while (!_rows.empty() && _rows.back().TimeMs >= from)
        {
            _rows.pop_back();
            ++dropped;
        }
        return dropped;
    }

    char const* ActivityName(Activity activity)
    {
        switch (activity)
        {
            case Activity::Idle: return "idle";
            case Activity::Travel: return "travel";
            case Activity::Questing: return "questing";
            case Activity::Gathering: return "gathering";
            case Activity::Vendor: return "vendor";
            case Activity::Fight: return "fight";
        }
        return "idle";
    }

    Activity ClassifyActivity(ActivityInputs const& inputs)
    {
        if (inputs.InCombat)
            return Activity::Fight;
        if (inputs.Gathering)
            return Activity::Gathering;
        if (inputs.AtService)
            return Activity::Vendor;
        if (inputs.Idle)
            return Activity::Idle;
        if (inputs.Traveling)
            return Activity::Travel;
        return Activity::Questing;
    }

    bool EpisodeTracker::Update(Activity activity, uint8_t context, uint64_t nowMs, uint64_t holdMs)
    {
        bool start = _id == 0 || _broken || context != _context || (activity == Activity::Fight && _current != Activity::Fight);
        if (!start && activity != _current)
        {
            if (!_hasCandidate || _candidate != activity)
            {
                _hasCandidate = true;
                _candidate = activity;
                _candidateSinceMs = nowMs;
            }
            start = nowMs - _candidateSinceMs >= holdMs;
        }
        else if (activity == _current)
            _hasCandidate = false;

        if (!start)
            return false;
        ++_id;
        _current = activity;
        _context = context;
        // A held switch dates from when the new activity began, not from when it was confirmed.
        _startMs = !_broken && _hasCandidate && _candidate == activity ? _candidateSinceMs : nowMs;
        _hasCandidate = false;
        _broken = false;
        return true;
    }

    bool MoveChanged(MoveSample const& last, MoveSample const& now)
    {
        float dx = now.X - last.X, dy = now.Y - last.Y, dz = now.Z - last.Z;
        return dx * dx + dy * dy + dz * dz > 0.0025f                  // 5 cm
            || std::fabs(NormalizeAngle(now.O - last.O)) > 0.01f
            || now.Flags != last.Flags || now.Flags2 != last.Flags2;
    }

    bool MoveSampleDue(MoveSample const& last, uint64_t lastMs, MoveSample const& now, uint64_t nowMs, uint32_t intervalMs)
    {
        if (now.Flags != last.Flags || now.Flags2 != last.Flags2)
            return true;
        return nowMs - lastMs >= intervalMs && MoveChanged(last, now);
    }

    float NormalizeAngle(float angle)
    {
        if (!std::isfinite(angle))
            return 0.0f;
        angle = std::fmod(angle + Pi, 2.0f * Pi);
        if (angle < 0.0f)
            angle += 2.0f * Pi;
        return angle - Pi;
    }

    float RelativeAngle(float fromX, float fromY, float facing, float toX, float toY)
    {
        if (fromX == toX && fromY == toY)
            return 0.0f;
        return NormalizeAngle(std::atan2(toY - fromY, toX - fromX) - facing);
    }

    PackSummary SummarizePack(Point const& self, std::vector<Point> const& allies)
    {
        PackSummary pack;
        if (allies.empty())
            return pack;
        float sumX = 0.0f, sumY = 0.0f, sumSin = 0.0f, sumCos = 0.0f;
        for (Point const& ally : allies)
        {
            sumX += ally.X;
            sumY += ally.Y;
            sumSin += std::sin(ally.Facing);
            sumCos += std::cos(ally.Facing);
        }
        float n = static_cast<float>(allies.size());
        float cx = sumX / n, cy = sumY / n;
        pack.Count = static_cast<uint32_t>(allies.size());
        pack.Distance = std::sqrt((cx - self.X) * (cx - self.X) + (cy - self.Y) * (cy - self.Y));
        pack.Angle = RelativeAngle(self.X, self.Y, self.Facing, cx, cy);
        // Circular mean, so headings of 350 and 10 degrees average to 0, not 180.
        pack.Heading = NormalizeAngle(std::atan2(sumSin, sumCos) - self.Facing);
        return pack;
    }

    float CastProgress(int32_t totalMs, int32_t remainingMs)
    {
        if (totalMs <= 0)
            return 1.0f;
        float left = static_cast<float>(std::clamp(remainingMs, 0, totalMs));
        return 1.0f - left / static_cast<float>(totalMs);
    }

    void FightTracker::Start(uint64_t nowMs)
    {
        if (_active)
            return;
        _active = true;
        _startMs = nowMs;
        _current = FightSummary();
    }

    FightSummary FightTracker::End(uint64_t nowMs)
    {
        FightSummary summary = _current;
        if (!_active)
            return summary;
        summary.DurationMs = nowMs > _startMs ? nowMs - _startMs : 0;
        _active = false;
        _ended = true;
        _endMs = nowMs;
        _current = FightSummary();
        return summary;
    }

    bool FightTracker::SinceEnd(uint64_t nowMs, uint64_t& sinceMs) const
    {
        if (_active || !_ended)
            return false;
        sinceMs = nowMs > _endMs ? nowMs - _endMs : 0;
        return true;
    }
}
