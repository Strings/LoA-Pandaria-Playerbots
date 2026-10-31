/*
 * Twink gear selection. See TwinkGearRules.h.
 */

#include "TwinkGearRules.h"

#include <algorithm>
#include <iterator>
#include <optional>
#include <tuple>

namespace TwinkGear
{
namespace
{
    // Core values (ItemPrototype.h, Player.h).
    constexpr uint8_t ClassWeapon = 2, ClassArmor = 4;
    constexpr uint8_t InvHead = 1, InvNeck = 2, InvShoulders = 3, InvChest = 5, InvWaist = 6, InvLegs = 7, InvFeet = 8,
        InvWrists = 9, InvHands = 10, InvFinger = 11, InvTrinket = 12, InvWeapon = 13, InvShield = 14, InvRanged = 15,
        InvCloak = 16, InvTwoHand = 17, InvRobe = 20, InvMainHand = 21, InvOffHand = 22, InvHoldable = 23,
        InvRangedRight = 26;
    constexpr uint8_t WeaponThrown = 16, WeaponWand = 19;
    constexpr uint8_t SlotHead = 0, SlotNeck = 1, SlotShoulders = 2, SlotChest = 4, SlotWaist = 5, SlotLegs = 6,
        SlotFeet = 7, SlotWrists = 8, SlotHands = 9, SlotFinger1 = 10, SlotFinger2 = 11, SlotTrinket1 = 12,
        SlotTrinket2 = 13, SlotBack = 14, SlotMainHand = 15, SlotOffHand = 16;

    struct SpecRow
    {
        uint32_t Spec;
        uint8_t Stat;
        WeaponStyle Style;
    };

    constexpr SpecRow Specs[] =
    {
        { 71, StatStrength, WeaponStyle::TwoHand },        // Arms
        { 72, StatStrength, WeaponStyle::TwoHand },        // Fury
        { 73, StatStrength, WeaponStyle::OneHandShield },  // Protection warrior
        { 65, StatIntellect, WeaponStyle::OneHandShield }, // Holy paladin (no staves)
        { 66, StatStrength, WeaponStyle::OneHandShield },  // Protection paladin
        { 70, StatStrength, WeaponStyle::TwoHand },        // Retribution
        { 253, StatAgility, WeaponStyle::Ranged },         // Beast Mastery
        { 254, StatAgility, WeaponStyle::Ranged },         // Marksmanship
        { 255, StatAgility, WeaponStyle::Ranged },         // Survival
        { 259, StatAgility, WeaponStyle::DualWield },      // Assassination
        { 260, StatAgility, WeaponStyle::DualWield },      // Combat
        { 261, StatAgility, WeaponStyle::DualWield },      // Subtlety
        { 256, StatIntellect, WeaponStyle::TwoHand },      // Discipline
        { 257, StatIntellect, WeaponStyle::TwoHand },      // Holy priest
        { 258, StatIntellect, WeaponStyle::TwoHand },      // Shadow
        { 250, StatStrength, WeaponStyle::TwoHand },       // Blood
        { 251, StatStrength, WeaponStyle::TwoHand },       // Frost death knight
        { 252, StatStrength, WeaponStyle::TwoHand },       // Unholy
        { 262, StatIntellect, WeaponStyle::TwoHand },      // Elemental
        { 263, StatAgility, WeaponStyle::DualWield },      // Enhancement
        { 264, StatIntellect, WeaponStyle::OneHandShield },// Restoration shaman
        { 62, StatIntellect, WeaponStyle::TwoHand },       // Arcane
        { 63, StatIntellect, WeaponStyle::TwoHand },       // Fire
        { 64, StatIntellect, WeaponStyle::TwoHand },       // Frost mage
        { 265, StatIntellect, WeaponStyle::TwoHand },      // Affliction
        { 266, StatIntellect, WeaponStyle::TwoHand },      // Demonology
        { 267, StatIntellect, WeaponStyle::TwoHand },      // Destruction
        { 268, StatAgility, WeaponStyle::TwoHand },        // Brewmaster
        { 269, StatAgility, WeaponStyle::TwoHand },        // Windwalker
        { 270, StatIntellect, WeaponStyle::TwoHand },      // Mistweaver
        { 102, StatIntellect, WeaponStyle::TwoHand },      // Balance
        { 103, StatAgility, WeaponStyle::TwoHand },        // Feral
        { 104, StatAgility, WeaponStyle::TwoHand },        // Guardian
        { 105, StatIntellect, WeaponStyle::TwoHand },      // Restoration druid
    };

    // First spec per class (index = class id), used before a spec is chosen.
    constexpr uint32_t DefaultSpec[] = { 0, 71, 70, 253, 259, 256, 250, 262, 62, 265, 268, 102 };
    constexpr uint8_t ClassArmorType[] = { ArmorCloth, ArmorPlate, ArmorPlate, ArmorMail, ArmorLeather, ArmorCloth,
        ArmorPlate, ArmorMail, ArmorCloth, ArmorCloth, ArmorLeather, ArmorLeather };

    uint32_t Score(Candidate const& c)
    {
        return c.ItemLevel * 10 + c.Quality;
    }

    bool StatFits(Profile const& profile, Candidate const& c)
    {
        return c.Stats == 0 || (c.Stats & profile.Stat) != 0;
    }

    template <typename Filter>
    std::optional<Candidate> Best(std::vector<Candidate> const& candidates, Profile const& profile, Filter filter,
        uint32_t exclude = 0, bool armorTypeFirst = false)
    {
        std::optional<Candidate> best;
        auto key = [&](Candidate const& c)
        {
            // Armor slots prefer the heaviest armor the class wears (its own type, then the next lower) at any item level.
            uint32_t weight = armorTypeFirst ? c.SubClass : 0u;
            return std::make_pair(weight, Score(c));
        };
        for (Candidate const& c : candidates)
        {
            if (c.Entry == exclude || !StatFits(profile, c) || !filter(c))
                continue;
            if (!best || key(c) > key(*best))
                best = c;
        }
        return best;
    }

    bool IsWeapon(Candidate const& c)
    {
        return c.ItemClass == ClassWeapon && c.SubClass != WeaponThrown && c.SubClass != WeaponWand;
    }

    bool IsOneHand(Candidate const& c)
    {
        return IsWeapon(c) && (c.InventoryType == InvWeapon || c.InventoryType == InvMainHand);
    }
}

Profile ProfileFor(uint32_t classId, uint32_t specId)
{
    Profile profile;
    if (classId < std::size(ClassArmorType))
        profile.Armor = ClassArmorType[classId];
    if (!specId && classId < std::size(DefaultSpec))
        specId = DefaultSpec[classId];
    for (SpecRow const& row : Specs)
    {
        if (row.Spec == specId)
        {
            profile.Stat = row.Stat;
            profile.Style = row.Style;
            break;
        }
    }
    return profile;
}

std::vector<Candidate> WithinLevel(std::vector<Candidate> const& candidates, uint32_t level)
{
    uint32_t reference = 0;
    uint32_t lowest = level > 11 ? level - 9 : 2;
    for (Candidate const& c : candidates)
        if (c.RequiredLevel >= lowest && c.RequiredLevel <= level)
            reference = std::max(reference, c.ItemLevel);
    if (!reference)
        return candidates;

    // 15% headroom: tuned twink items sit about 7% above their heirloom.
    uint32_t cap = reference * 115 / 100;
    std::vector<Candidate> kept;
    for (Candidate const& c : candidates)
        if (c.ItemLevel <= cap)
            kept.push_back(c);
    return kept;
}

std::vector<Pick> Select(Profile const& profile, std::vector<Candidate> const& candidates)
{
    std::vector<Pick> picks;
    auto add = [&picks](uint8_t slot, std::optional<Candidate> const& c)
    {
        if (c)
            picks.push_back({ slot, c->Entry });
    };

    struct ArmorSlot
    {
        uint8_t Slot;
        uint8_t Inv;
        uint8_t AltInv;
    };
    static constexpr ArmorSlot armorSlots[] =
    {
        { SlotHead, InvHead, InvHead }, { SlotShoulders, InvShoulders, InvShoulders }, { SlotChest, InvChest, InvRobe },
        { SlotWaist, InvWaist, InvWaist }, { SlotLegs, InvLegs, InvLegs }, { SlotFeet, InvFeet, InvFeet },
        { SlotWrists, InvWrists, InvWrists }, { SlotHands, InvHands, InvHands },
    };
    for (ArmorSlot const& slot : armorSlots)
        add(slot.Slot, Best(candidates, profile, [&](Candidate const& c)
            {
                return c.ItemClass == ClassArmor && c.SubClass >= ArmorCloth && c.SubClass <= profile.Armor &&
                    (c.InventoryType == slot.Inv || c.InventoryType == slot.AltInv);
            }, 0, true));

    add(SlotNeck, Best(candidates, profile, [](Candidate const& c) { return c.InventoryType == InvNeck; }));
    add(SlotBack, Best(candidates, profile, [](Candidate const& c) { return c.InventoryType == InvCloak; }));
    for (auto [first, second, inv] : { std::make_tuple(SlotFinger1, SlotFinger2, InvFinger), std::make_tuple(SlotTrinket1, SlotTrinket2, InvTrinket) })
    {
        auto one = Best(candidates, profile, [inv = inv](Candidate const& c) { return c.InventoryType == inv; });
        add(first, one);
        if (one)
            add(second, Best(candidates, profile, [inv = inv](Candidate const& c) { return c.InventoryType == inv; }, one->Entry));
    }

    // Weapons: the spec's style first, then the other styles in a fixed order, so a slot is never left empty when
    // something usable exists.
    auto twoHand = Best(candidates, profile, [](Candidate const& c) { return IsWeapon(c) && c.InventoryType == InvTwoHand; });
    auto ranged = Best(candidates, profile, [](Candidate const& c)
        {
            return IsWeapon(c) && (c.InventoryType == InvRanged || c.InventoryType == InvRangedRight);
        });
    auto mainHand = Best(candidates, profile, IsOneHand);
    auto offWeapon = Best(candidates, profile, [](Candidate const& c)
        {
            return IsWeapon(c) && (c.InventoryType == InvWeapon || c.InventoryType == InvOffHand);
        }, mainHand ? mainHand->Entry : 0);
    if (!offWeapon && mainHand && mainHand->InventoryType == InvWeapon)
        offWeapon = mainHand;                                    // a second copy of the same one-hander
    auto shield = Best(candidates, profile, [](Candidate const& c) { return c.InventoryType == InvShield; });
    auto holdable = Best(candidates, profile, [](Candidate const& c) { return c.InventoryType == InvHoldable; });

    auto oneHandWith = [&](std::optional<Candidate> const& off)
    {
        add(SlotMainHand, mainHand);
        add(SlotOffHand, off);
    };
    switch (profile.Style)
    {
        case WeaponStyle::Ranged:
            if (ranged)
                add(SlotMainHand, ranged);
            else if (twoHand)
                add(SlotMainHand, twoHand);
            else
                oneHandWith(offWeapon);
            break;
        case WeaponStyle::DualWield:
            if (mainHand)
                oneHandWith(offWeapon);
            else
                add(SlotMainHand, twoHand);
            break;
        case WeaponStyle::OneHandShield:
            if (mainHand)
                oneHandWith(shield ? shield : holdable);
            else
                add(SlotMainHand, twoHand);
            break;
        case WeaponStyle::TwoHand:
            if (twoHand)
                add(SlotMainHand, twoHand);
            else if (mainHand)
                oneHandWith(profile.Stat == StatIntellect ? holdable : offWeapon);
            break;
    }
    return picks;
}
}
