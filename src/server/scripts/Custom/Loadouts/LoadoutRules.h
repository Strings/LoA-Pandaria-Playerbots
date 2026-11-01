/*
 * Spec loadouts (T-017): server-independent rules, kept out of LoadoutMgr so they can be unit tested.
 */

#ifndef ULTIMATEMOP_LOADOUT_RULES_H
#define ULTIMATEMOP_LOADOUT_RULES_H

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <string>
#include <vector>

namespace Loadouts
{
    constexpr uint8_t NoGroup = 255;          // a loadout not held by either client talent group
    constexpr std::size_t MaxNameLength = 24;

    // Names are shown in chat and sent to the addon tab-separated, so tabs, pipes and control characters are out.
    inline bool ValidName(std::string const& name)
    {
        if (name.empty() || name.size() > MaxNameLength)
            return false;
        if (name.front() == ' ' || name.back() == ' ')
            return false;
        for (unsigned char c : name)
            if (c < 0x20 || c == 0x7F || c == '|' || c == '\t')
                return false;
        // A name made only of digits would read as an id.
        return !std::all_of(name.begin(), name.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
    }

    inline bool SameName(std::string const& a, std::string const& b)
    {
        return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(),
            [](unsigned char x, unsigned char y) { return std::tolower(x) == std::tolower(y); });
    }

    // The client keeps exactly two talent groups. A loadout is written into the one that is not active.
    inline uint8_t TargetGroup(uint8_t activeGroup)
    {
        return activeGroup == 0 ? 1 : 0;
    }

    // Specs of the class that still need their default loadout (one per spec, in the class's spec order).
    inline std::vector<uint16_t> MissingDefaults(std::vector<uint16_t> const& classSpecs, std::vector<uint16_t> const& defaultSpecs)
    {
        std::vector<uint16_t> missing;
        for (uint16_t spec : classSpecs)
            if (spec && std::find(defaultSpecs.begin(), defaultSpecs.end(), spec) == defaultSpecs.end())
                missing.push_back(spec);
        return missing;
    }

    // The smallest id above every id in use, so ids stay stable and are never reused while a row exists.
    inline uint32_t NextId(std::vector<uint32_t> const& ids)
    {
        uint32_t next = 1;
        for (uint32_t id : ids)
            next = std::max(next, id + 1);
        return next;
    }

    struct Entry
    {
        uint32_t Id = 0;
        std::string Name;
    };

    // Finds a loadout by id ("3") or by name, ignoring case. Returns 0 when nothing or more than one matches.
    inline uint32_t Resolve(std::string const& arg, std::vector<Entry> const& entries)
    {
        if (arg.empty())
            return 0;
        if (std::all_of(arg.begin(), arg.end(), [](unsigned char c) { return std::isdigit(c) != 0; }) && arg.size() < 10)
        {
            uint32_t id = uint32_t(std::stoul(arg));
            for (Entry const& entry : entries)
                if (entry.Id == id)
                    return id;
            return 0;
        }
        uint32_t found = 0;
        for (Entry const& entry : entries)
        {
            if (!SameName(entry.Name, arg))
                continue;
            if (found)
                return 0;
            found = entry.Id;
        }
        return found;
    }
}

#endif
