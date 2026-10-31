/*
 * Twink gear selection: which candidate goes in which slot for a class and spec. Pure logic (no server headers),
 * unit-tested in tests/gmtools/. The constants mirror the core's ItemClass, InventoryType and EquipmentSlots
 * values; the caller filters out items the character cannot use before calling Select.
 */

#ifndef ULTIMATEMOP_TWINK_GEAR_RULES_H
#define ULTIMATEMOP_TWINK_GEAR_RULES_H

#include <cstdint>
#include <vector>

namespace TwinkGear
{
    // Primary stats, as a bitmask; an item with none fits every spec.
    constexpr uint8_t StatStrength = 1;
    constexpr uint8_t StatAgility = 2;
    constexpr uint8_t StatIntellect = 4;          // intellect or spell power

    constexpr uint8_t ArmorCloth = 1, ArmorLeather = 2, ArmorMail = 3, ArmorPlate = 4;

    enum class WeaponStyle : uint8_t
    {
        TwoHand,                                  // casters get a staff, falling back to one-hand + off-hand
        DualWield,
        OneHandShield,
        Ranged,
    };

    struct Profile
    {
        uint8_t Armor = ArmorCloth;               // the class's own armor type
        uint8_t Stat = StatIntellect;
        WeaponStyle Style = WeaponStyle::TwoHand;
    };

    // Specialization ids from ChrSpecialization.dbc; spec 0 (none yet) uses the class's first spec.
    Profile ProfileFor(uint32_t classId, uint32_t specId);

    struct Candidate
    {
        uint32_t Entry = 0;
        uint8_t ItemClass = 0;                    // 2 weapon, 4 armor
        uint8_t SubClass = 0;
        uint8_t InventoryType = 0;
        uint32_t ItemLevel = 0;
        uint8_t Quality = 0;
        uint8_t Stats = 0;                        // Stat* bits
        uint8_t RequiredLevel = 0;
    };

    struct Pick
    {
        uint8_t Slot = 0;                         // EquipmentSlots
        uint32_t Entry = 0;
    };

    // Drops outliers: items far above the item level that properly leveled items (required level within ten levels
    // below) have at this level. Quest rewards without a level requirement, toys and PvP items otherwise outrank
    // everything. Without any leveled reference item nothing is dropped.
    std::vector<Candidate> WithinLevel(std::vector<Candidate> const& candidates, uint32_t level);

    // Best usable candidate per equipment slot, main hand before off hand.
    std::vector<Pick> Select(Profile const& profile, std::vector<Candidate> const& candidates);
}

#endif
