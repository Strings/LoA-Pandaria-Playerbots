#include "TestMain.h"
#include "HeartseedRules.h"

using namespace Heartseed;

namespace
{
    void MinorsByCharacterLevel()
    {
        std::array<uint8_t, MaxPaths> majors = {};
        UMP_CHECK(MinorActive(Path::None, 1, 1, majors));
        UMP_CHECK(!MinorActive(Path::Xuen, 1, 90, majors));          // no major on the path yet
        majors[uint8_t(Path::Xuen)] = 1;
        UMP_CHECK(MinorActive(Path::Xuen, 20, 20, majors) && !MinorActive(Path::Xuen, 20, 19, majors));
    }

    void GrowthXpLevels()
    {
        UMP_CHECK(GrowthXpForLevel(0, 200, 50) == 200 && GrowthXpForLevel(3, 200, 50) == 350);
        UMP_CHECK(GrowthXpForLevel(0, 0, 0) == 1);                   // never free
        UMP_CHECK(GrowthXpForLevel(UINT32_MAX, UINT32_MAX, UINT32_MAX) == UINT32_MAX);
        GrowthProgress p = ApplyGrowthXp(0, 499, 200, 50);           // 200 + 250 = 450 for two levels
        UMP_CHECK(p.Level == 2 && p.Xp == 49);
        p = ApplyGrowthXp(40, 0, 200, 50);
        UMP_CHECK(p.Level == 40 && p.Xp == 0);
        p = ApplyGrowthXp(UINT32_MAX, UINT32_MAX, 1, 0);             // no wrap at the top
        UMP_CHECK(p.Level == UINT32_MAX);

        UMP_CHECK(GrowthSourceNearLevel(85, 80, 5) && !GrowthSourceNearLevel(85, 79, 5));
        UMP_CHECK(GrowthSourceNearLevel(20, 90, 0));                 // a harder source always counts
    }

    void GrowthPointsAndAmounts()
    {
        std::map<uint8_t, uint32_t> spent = { { 1, 2 }, { 3, 1 } };
        UMP_CHECK(UnspentPoints(5, spent) == 2);
        UMP_CHECK(UnspentPoints(2, spent) == 0);                     // over-spent (GM) never wraps
        UMP_CHECK(UnspentPoints(4, {}) == 4);
        UMP_CHECK(StatPointAmount(3, 1) == 3 && StatPointAmount(3, 0) == 0 && StatPointAmount(3, -2) == 0);
        UMP_CHECK(StatPointAmount(UINT32_MAX, 2) == INT32_MAX);

        UMP_CHECK(MainStatFor(10, 268) == MainStat::Agility && MainStatFor(10, 270) == MainStat::Intellect);
        UMP_CHECK(MainStatFor(2, 66) == MainStat::Strength && MainStatFor(2, 65) == MainStat::Intellect);
        UMP_CHECK(MainStatFor(11, 0) == MainStat::Intellect && MainStatFor(4, 0) == MainStat::Agility);
        UMP_CHECK(MainStatFor(1, 0) == MainStat::Strength);
    }

    void DefaultUnlockLevels()
    {
        UMP_CHECK(MajorTierLevel(0) == 10 && MajorTierLevel(5) == 85 && MajorTierLevel(MaxMajorTiers) == 0);
        UMP_CHECK(MajorTierForLevel(25) == 1 && MajorTierForLevel(26) == MaxMajorTiers);
        UMP_CHECK(!MajorTierOpen(2, 39) && MajorTierOpen(2, 40));
        UMP_CHECK(EssenceSlotsForLevel(9) == 0 && EssenceSlotsForLevel(40) == 2 && EssenceSlotsForLevel(90) == 3);
    }

    void ConfiguredUnlockLevels()
    {
        UnlockLevels saved = Unlocks();
        UMP_CHECK(SetLevels(Unlocks().MajorTiers, { 5, 15, 30, 45, 60, 75 }));
        UMP_CHECK(SetLevels(Unlocks().EssenceSlots, { 1, 1, 50 }));   // equal levels are allowed
        UMP_CHECK(MajorTierForLevel(15) == 1 && MajorTierOpen(0, 5) && !MajorTierOpen(5, 74));
        UMP_CHECK(EssenceSlotsForLevel(1) == 2 && FreeEssenceSlot({ 1 }, 1) == 2 && FreeEssenceSlot({ 1, 2 }, 49) == 0);
        Unlocks() = saved;
    }

    void RejectedUnlockLevels()
    {
        UnlockLevels saved = Unlocks();
        std::array<uint8_t, MaxEssenceSlots> before = Unlocks().EssenceSlots;
        UMP_CHECK(!SetLevels(Unlocks().EssenceSlots, { 10, 40 }));        // wrong count
        UMP_CHECK(!SetLevels(Unlocks().EssenceSlots, { 40, 10, 70 }));    // not ascending
        UMP_CHECK(!SetLevels(Unlocks().EssenceSlots, { 0, 10, 70 }));     // level 0
        UMP_CHECK(!SetLevels(Unlocks().EssenceSlots, { 10, 40, 256 }));   // past a level byte
        UMP_CHECK(Unlocks().EssenceSlots == before);
        Unlocks() = saved;
    }

    void GiftRidersByStage()
    {
        std::vector<GiftRider> riders = {
            { 2, GiftRiderKind::SelfAura, 15, 10000, 26297 },
            { 3, GiftRiderKind::ExtraTargets, 2, 0, 0 },
            { 4, GiftRiderKind::KillCooldown, 1000, 0, 0 } };
        UMP_CHECK(ActiveRiders(riders, 1).empty());                  // Sprout: the bare Gift
        UMP_CHECK(ActiveRiders(riders, 3).size() == 2);              // Bloom keeps Sapling's rider
        UMP_CHECK(ActiveRiders(riders, 5).size() == 3);              // Celestial Weapon keeps all three
        UMP_CHECK(ActiveRiders(riders, 3)[1].Kind == GiftRiderKind::ExtraTargets);
    }

    void GiftRiderAmounts()
    {
        UMP_CHECK(PercentOf(200000, 10) == 20000);
        UMP_CHECK(PercentOf(1000, 0) == 0 && PercentOf(1000, -5) == 0);
        UMP_CHECK(PercentOf(UINT32_MAX, 200) == UINT32_MAX);
        UMP_CHECK(CooldownCut(20000, 1000) == 1000);
        UMP_CHECK(CooldownCut(500, 1500) == 500);                    // never past ready
        UMP_CHECK(CooldownCut(17000, 0) == 17000);                   // 0 is a full reset
    }

    void MeterStacksAndDecay()
    {
        MeterRule thunder;
        thunder.Source = MeterSource::Crit;
        thunder.MaxStacks = 5;
        thunder.DecayMs = 15000;
        MeterState meter;
        for (int i = 0; i < 7; ++i)
            MeterBuild(meter, thunder, 1000 + i);
        UMP_CHECK(meter.Stacks == 5);                                // capped
        MeterDecay(meter, thunder, 1006 + 14999);
        UMP_CHECK(meter.Stacks == 5);
        MeterDecay(meter, thunder, 1006 + 15000);
        UMP_CHECK(meter.Stacks == 0);                                // idle too long

        thunder.BuildCooldownMs = 500;
        MeterState limited;
        UMP_CHECK(MeterBuild(limited, thunder, 0));
        UMP_CHECK(!MeterBuild(limited, thunder, 499));               // AoE crits in one swing count once
        UMP_CHECK(MeterBuild(limited, thunder, 500) && limited.Stacks == 2);

        // getMSTime() wraps; the meter still decays across it.
        MeterState wrapped;
        MeterBuild(wrapped, thunder, UINT32_MAX - 100);
        MeterDecay(wrapped, thunder, 20000);
        UMP_CHECK(wrapped.Stacks == 0);
    }

    void MeterSpendAndScale()
    {
        MeterRule coil;
        coil.MaxStacks = 5;
        coil.SpendBonusPct = 20;
        MeterState meter;
        MeterBuild(meter, coil, 0, 3);
        UMP_CHECK(MeterSpend(meter, coil, 10) == 3 && meter.Stacks == 0);
        UMP_CHECK(SpendScaled(1000, 3, 20) == 1600);
        UMP_CHECK(SpendScaled(1000, 0, 20) == 1000);
        UMP_CHECK(SpendScaled(1000, 5, -30) == 0);                   // never negative
        UMP_CHECK(SpendScaled(UINT32_MAX, 5, 100) == UINT32_MAX);
    }

    void MeterHealingAndBigHits()
    {
        MeterRule hope;
        hope.Source = MeterSource::Healing;
        hope.MaxStacks = 5;
        hope.ThresholdPct = 5;
        MeterState meter;
        UMP_CHECK(HealingStacks(meter, hope, 400, 10000) == 0 && meter.HealAccum == 400);
        UMP_CHECK(HealingStacks(meter, hope, 1700, 10000) == 4 && meter.HealAccum == 100);   // carries over
        UMP_CHECK(HealingStacks(meter, hope, 5000, 0) == 0);         // no health, no Hope

        MeterRule retort;
        retort.Source = MeterSource::BigHit;
        retort.ThresholdPct = 8;
        retort.LitMs = 6000;
        UMP_CHECK(!IsBigHit(retort, 799, 10000) && IsBigHit(retort, 800, 10000));
        MeterState lit;
        UMP_CHECK(!MeterAllowsCast(lit, retort, 0));                 // gated until a big hit
        MeterBuild(lit, retort, 1000);
        UMP_CHECK(MeterAllowsCast(lit, retort, 6999));
        UMP_CHECK(!MeterAllowsCast(lit, retort, 7000));              // the window ran out
        MeterSpend(lit, retort, 2000);
        UMP_CHECK(!MeterAllowsCast(lit, retort, 2001));              // casting uses it up
        UMP_CHECK(MeterAllowsCast(MeterState(), MeterRule(), 0));    // ungated Gifts always cast
    }

    void MeterCritCutAndRecentDamage()
    {
        MeterRule rule;
        rule.CritCooldownCutMs = 500;
        rule.BuildCooldownMs = 1000;
        MeterState meter;
        UMP_CHECK(CritCut(meter, rule, 0) == 500);
        UMP_CHECK(CritCut(meter, rule, 999) == 0);
        UMP_CHECK(CritCut(meter, rule, 1000) == 500);
        UMP_CHECK(CritCut(meter, MeterRule(), 5000) == 0);

        std::vector<std::pair<uint32_t, uint32_t>> hits = { { 0, 100 }, { 3000, 200 }, { 5000, 300 } };
        UMP_CHECK(RecentDamage(hits, 5000, 5000) == 600);            // the edge of the window counts
        UMP_CHECK(RecentDamage(hits, 8001, 5000) == 300 && hits.size() == 1);
    }
}

void RunTests()
{
    MinorsByCharacterLevel();
    GrowthXpLevels();
    GrowthPointsAndAmounts();
    DefaultUnlockLevels();
    ConfiguredUnlockLevels();
    RejectedUnlockLevels();
    GiftRidersByStage();
    GiftRiderAmounts();
    MeterStacksAndDecay();
    MeterSpendAndScale();
    MeterHealingAndBigHits();
    MeterCritCutAndRecentDamage();
}
