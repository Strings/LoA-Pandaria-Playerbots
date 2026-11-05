#ifndef ULTIMATEMOP_PROFESSION_TOME_RULES_H
#define ULTIMATEMOP_PROFESSION_TOME_RULES_H
#include <array>
#include <cstdint>
// Server-independent data for the profession tome pet, kept apart from the script so it can be unit tested.
namespace ProfessionTome
{
    // Gusting Grimoire: a stock MoP journal pet (an animated tome), so the client already knows its
    // species, model, icon and summon spell. Its creature gets gossip and trainer flags from our SQL.
    constexpr std::uint32_t Species = 1174;
    constexpr std::uint32_t Creature = 69208;
    constexpr std::uint32_t SummonSpell = 136484;

    struct Profession { std::uint32_t Skill; char const* Name; bool Primary; };
    // Gossip order: primaries alphabetically, then secondaries. Skill ids match ProfessionRules::Primary.
    constexpr std::array<Profession, 15> Professions = {{
        {171, "Alchemy", true}, {164, "Blacksmithing", true}, {333, "Enchanting", true},
        {202, "Engineering", true}, {182, "Herbalism", true}, {773, "Inscription", true},
        {755, "Jewelcrafting", true}, {165, "Leatherworking", true}, {186, "Mining", true},
        {393, "Skinning", true}, {197, "Tailoring", true},
        {794, "Archaeology", false}, {185, "Cooking", false}, {129, "First Aid", false}, {356, "Fishing", false}
    }};

    constexpr Profession const* Find(std::uint32_t skill)
    {
        for (Profession const& profession : Professions)
            if (profession.Skill == skill)
                return &profession;
        return nullptr;
    }
}
#endif
