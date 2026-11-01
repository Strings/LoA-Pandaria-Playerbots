#ifndef ULTIMATEMOP_PROFESSION_RULES_H
#define ULTIMATEMOP_PROFESSION_RULES_H
#include <array>
#include <cstdint>
namespace ProfessionRules
{
    struct Profession { std::uint32_t Skill; std::uint32_t FirstSpell; };
    constexpr std::array<Profession, 11> Primary = {{
        {171, 2259}, {164, 2018}, {333, 7411}, {202, 4036}, {182, 2366},
        {773, 45357}, {755, 25229}, {165, 2108}, {186, 2575}, {393, 8613}, {197, 3908}
    }};
    constexpr std::uint32_t FreeSlots(std::uint32_t configured, std::uint32_t known)
    {
        std::uint32_t cap = configured < Primary.size() ? configured : std::uint32_t(Primary.size());
        return cap > known ? cap - known : 0;
    }
}
#endif
