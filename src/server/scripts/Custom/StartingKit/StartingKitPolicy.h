#ifndef ULTIMATEMOP_STARTING_KIT_POLICY_H
#define ULTIMATEMOP_STARTING_KIT_POLICY_H
#include <cstdint>
namespace StartingKit
{
    constexpr std::uint32_t MountForRace(std::uint8_t race)
    {
        switch (race)
        {
            case 1: return 458;    // Brown Horse
            case 2: return 6654;   // Brown Wolf
            case 3: return 6899;   // Brown Ram
            case 4: return 8394;   // Striped Frostsaber
            case 5: return 17462;  // Red Skeletal Horse
            case 6: return 18989;  // Gray Kodo
            case 7: return 10969;  // Blue Mechanostrider
            case 8: return 10796;  // Turquoise Raptor
            case 9: return 87090;  // Goblin Trike
            case 10: return 34795; // Red Hawkstrider
            case 11: return 34406; // Brown Elekk
            case 22: return 103195; // Mountain Horse
            case 24: case 25: case 26: return 120395; // Green Dragon Turtle, including neutral Pandaren
            default: return 0;
        }
    }
}
#endif
