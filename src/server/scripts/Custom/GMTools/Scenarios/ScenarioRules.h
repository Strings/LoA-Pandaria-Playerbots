/*
 * Pure part of the scenario loader (T-039): placeholder expansion and argument splitting, free of server headers
 * so the unit tests can build it alone.
 */

#ifndef ULTIMATEMOP_SCENARIO_RULES_H
#define ULTIMATEMOP_SCENARIO_RULES_H

#include <map>
#include <string>
#include <vector>

namespace Scenarios
{
    // Splits on spaces, dropping empty pieces.
    inline std::vector<std::string> SplitArgs(std::string const& text)
    {
        std::vector<std::string> out;
        std::string current;
        for (char c : text)
        {
            if (c == ' ' || c == '\t')
            {
                if (!current.empty())
                    out.push_back(current);
                current.clear();
            }
            else
                current += c;
        }
        if (!current.empty())
            out.push_back(current);
        return out;
    }

    // Replaces each {key} with its value. An unknown or empty placeholder fails the whole step, naming it in
    // `missing`, so a scenario never runs a half-filled command. "{{" writes a literal "{".
    inline bool Expand(std::string const& text, std::map<std::string, std::string> const& values, std::string& out,
        std::string& missing)
    {
        out.clear();
        for (size_t i = 0; i < text.size(); ++i)
        {
            if (text[i] != '{')
            {
                out += text[i];
                continue;
            }
            if (i + 1 < text.size() && text[i + 1] == '{')
            {
                out += '{';
                ++i;
                continue;
            }

            size_t close = text.find('}', i);
            if (close == std::string::npos)
            {
                missing = text.substr(i);
                return false;
            }

            std::string key = text.substr(i + 1, close - i - 1);
            auto itr = values.find(key);
            if (itr == values.end() || itr->second.empty())
            {
                missing = key;
                return false;
            }
            out += itr->second;
            i = close;
        }
        return true;
    }

    // A step may not start another scenario: loops would be easy to write and hard to stop.
    inline bool IsAllowedStep(std::string const& command)
    {
        std::string text = command;
        size_t start = text.find_first_not_of(" .");
        if (start == std::string::npos)
            return false;
        text = text.substr(start);
        return text.compare(0, 8, "scenario") != 0;
    }
}

#endif
