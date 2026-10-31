/*
 * Bot inspection registry (T-039). Each bot system registers the sections it can explain (`traits`, `memory`,
 * `skill`, ...) at startup; `.bot <section> [name]` asks the owner. A section with no owner yet answers that its
 * system has not been built, so the command surface is stable from the start.
 */

#ifndef _PLAYERBOT_BOTINSPECT_H
#define _PLAYERBOT_BOTINSPECT_H

#include <functional>
#include <string>
#include <vector>

class Player;

namespace BotInspect
{
    // Appends human-readable lines about `bot`. Runs on the world thread.
    using Section = std::function<void(Player* bot, std::vector<std::string>& lines)>;

    // Registers once at startup; false if the name is taken.
    bool Register(std::string const& name, std::string const& description, Section section);

    // Sections the `.bot` command reserves before their systems exist, with the task that will fill them.
    struct Planned
    {
        char const* Name;
        char const* Arrives;
    };
    std::vector<Planned> const& PlannedSections();

    // False when no system owns `name` (lines then say so).
    bool Explain(std::string const& name, Player* bot, std::vector<std::string>& lines);

    // Last thing the bot did, traced or not: the trace's chosen action when on, else the engine's last action.
    // For capture tickets and other tools that need a one-line answer.
    std::string LastDecisionSummary(Player* bot);
}

#endif
