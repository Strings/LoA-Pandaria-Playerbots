#include "TestMain.h"
#include "TwinkGearRules.h"

using namespace TwinkGear;

namespace
{
    // Core values: item class 2 weapon / 4 armor; inventory types; equipment slots.
    Candidate Armor(uint32_t entry, uint8_t subClass, uint8_t inv, uint32_t ilvl, uint8_t stats)
    {
        return { entry, 4, subClass, inv, ilvl, 2, stats };
    }

    Candidate Weapon(uint32_t entry, uint8_t subClass, uint8_t inv, uint32_t ilvl, uint8_t stats)
    {
        return { entry, 2, subClass, inv, ilvl, 2, stats };
    }

    uint32_t At(std::vector<Pick> const& picks, uint8_t slot)
    {
        for (Pick const& pick : picks)
            if (pick.Slot == slot)
                return pick.Entry;
        return 0;
    }

    void Profiles()
    {
        Profile fury = ProfileFor(1, 72);
        UMP_CHECK(fury.Armor == ArmorPlate && fury.Stat == StatStrength && fury.Style == WeaponStyle::TwoHand);
        Profile holy = ProfileFor(2, 65);
        UMP_CHECK(holy.Stat == StatIntellect && holy.Style == WeaponStyle::OneHandShield);
        UMP_CHECK(ProfileFor(3, 0).Style == WeaponStyle::Ranged);               // no spec yet: first spec
        UMP_CHECK(ProfileFor(4, 0).Style == WeaponStyle::DualWield);
        UMP_CHECK(ProfileFor(11, 103).Armor == ArmorLeather && ProfileFor(11, 103).Stat == StatAgility);
        UMP_CHECK(ProfileFor(8, 0).Armor == ArmorCloth && ProfileFor(8, 0).Stat == StatIntellect);
    }

    void ArmorTypeAndStat()
    {
        Profile warrior = ProfileFor(1, 71);
        std::vector<Candidate> pool =
        {
            Armor(1, 3, 1, 30, StatStrength),        // mail helm, higher item level
            Armor(2, 4, 1, 25, StatStrength),        // plate helm: own armor type wins
            Armor(3, 4, 7, 40, StatIntellect),       // plate legs, wrong stat: skipped
            Armor(4, 3, 7, 20, StatStrength),        // mail legs: falls back below own type
            Armor(5, 2, 5, 50, 0),                   // leather chest with no primary stat: allowed
            Armor(6, 3, 20, 45, StatStrength),       // mail robe: fills the chest slot too
        };
        auto picks = Select(warrior, pool);
        UMP_CHECK(At(picks, 0) == 2);
        UMP_CHECK(At(picks, 6) == 4);
        UMP_CHECK(At(picks, 4) == 6);                // heavier armor first: the mail robe beats the higher leather chest
    }

    void RingsAndTrinkets()
    {
        Profile mage = ProfileFor(8, 63);
        std::vector<Candidate> pool =
        {
            Armor(10, 0, 11, 30, StatIntellect), Armor(11, 0, 11, 28, 0), Armor(12, 0, 11, 35, StatAgility),
            Armor(13, 0, 12, 30, 0),
        };
        auto picks = Select(mage, pool);
        UMP_CHECK(At(picks, 10) == 10 && At(picks, 11) == 11);    // two different rings; agility ring skipped
        UMP_CHECK(At(picks, 12) == 13 && At(picks, 13) == 0);      // one trinket only: no duplicate
    }

    void WeaponStyles()
    {
        std::vector<Candidate> pool =
        {
            Weapon(20, 1, 17, 30, StatStrength),     // two-hand axe
            Weapon(21, 7, 13, 28, StatAgility),      // one-hand sword
            Weapon(22, 15, 13, 26, StatAgility),     // dagger
            Weapon(23, 2, 15, 29, StatAgility),      // bow
            Weapon(24, 10, 17, 27, StatIntellect),   // staff
            Weapon(25, 19, 26, 40, StatIntellect),   // wand: never picked
            Armor(26, 6, 14, 25, 0),                 // shield
            Weapon(27, 4, 13, 24, StatIntellect),    // one-hand mace
        };
        UMP_CHECK(At(Select(ProfileFor(1, 71), pool), 15) == 20);
        auto rogue = Select(ProfileFor(4, 260), pool);
        UMP_CHECK(At(rogue, 15) == 21 && At(rogue, 16) == 22);
        UMP_CHECK(At(Select(ProfileFor(3, 254), pool), 15) == 23);
        auto mage = Select(ProfileFor(8, 63), pool);
        UMP_CHECK(At(mage, 15) == 24 && At(mage, 16) == 0);
        auto holy = Select(ProfileFor(2, 65), pool);
        UMP_CHECK(At(holy, 15) == 27 && At(holy, 16) == 26);
    }

    void Outliers()
    {
        Candidate leveled = Armor(40, 4, 1, 30, StatStrength);
        leveled.RequiredLevel = 25;
        Candidate quest = Armor(41, 4, 1, 34, StatStrength);          // within 15% of 30: kept
        Candidate toy = Armor(42, 0, 12, 400, 0);                     // item level 400 trinket, no requirement
        toy.RequiredLevel = 1;
        auto kept = WithinLevel({ leveled, quest, toy }, 29);
        UMP_CHECK(kept.size() == 2 && kept[0].Entry == 40 && kept[1].Entry == 41);
        UMP_CHECK(WithinLevel({ toy }, 29).size() == 1);              // no leveled reference: nothing dropped
    }

    void Fallbacks()
    {
        // A hunter with no ranged weapon still gets a melee weapon; a rogue with one one-hander dual wields two.
        std::vector<Candidate> pool = { Weapon(30, 7, 13, 20, StatAgility) };
        UMP_CHECK(At(Select(ProfileFor(3, 253), pool), 15) == 30);
        auto rogue = Select(ProfileFor(4, 259), pool);
        UMP_CHECK(At(rogue, 15) == 30 && At(rogue, 16) == 30);
        UMP_CHECK(Select(ProfileFor(1, 71), {}).empty());
    }
}

void RunTests()
{
    Profiles();
    ArmorTypeAndStat();
    RingsAndTrinkets();
    WeaponStyles();
    Outliers();
    Fallbacks();
}
