#include "WildRareRules.h"
#include "TestMain.h"
#include <cmath>
#include <string>
void RunTests()
{
    using namespace WildRares;

    // An ordinary hostile open-world mob is eligible.
    Facts mob;
    mob.HasSpawnId = mob.NormalRank = mob.Attackable = mob.Hostile = mob.Alive = true;
    UMP_CHECK(Judge(mob) == Verdict::Eligible);

    // Each exclusion on its own rules the mob out, with its own reason.
    auto with = [&](auto change) { Facts f = mob; change(f); return Judge(f); };
    UMP_CHECK(with([](Facts& f) { f.IsPlayerControlled = true; }) == Verdict::PlayerControlled);
    UMP_CHECK(with([](Facts& f) { f.HasSpawnId = false; }) == Verdict::NoSpawnPoint);
    UMP_CHECK(with([](Facts& f) { f.InInstance = true; }) == Verdict::Instance);
    UMP_CHECK(with([](Facts& f) { f.InCity = true; }) == Verdict::City);
    UMP_CHECK(with([](Facts& f) { f.HasNpcRole = true; }) == Verdict::NpcRole);
    UMP_CHECK(with([](Facts& f) { f.NormalRank = false; }) == Verdict::NotNormalRank);
    UMP_CHECK(with([](Facts& f) { f.Critter = true; }) == Verdict::Critter);
    UMP_CHECK(with([](Facts& f) { f.Attackable = false; }) == Verdict::NotAttackable);
    UMP_CHECK(with([](Facts& f) { f.Hostile = false; }) == Verdict::NotHostile);
    UMP_CHECK(with([](Facts& f) { f.Denied = true; }) == Verdict::Denied);
    UMP_CHECK(with([](Facts& f) { f.Alive = false; }) == Verdict::Dead);
    // A summoned pet reports as player-controlled before it reports the missing spawn point.
    UMP_CHECK(with([](Facts& f) { f.IsPlayerControlled = true; f.HasSpawnId = false; }) == Verdict::PlayerControlled);
    UMP_CHECK(std::string(Describe(Verdict::City)) == "in a city or sanctuary");

    // The roll: under the chance turns wild, at or over it misses.
    UMP_CHECK(RollSpawn(2.0f, 0, 6, false, 1.99f) == RollResult::Wild);
    UMP_CHECK(RollSpawn(2.0f, 0, 6, false, 2.0f) == RollResult::Missed);
    UMP_CHECK(RollSpawn(0.0f, 0, 6, false, 0.0f) == RollResult::Missed);
    UMP_CHECK(RollSpawn(100.0f, 0, 6, false, 99.99f) == RollResult::Wild);
    UMP_CHECK(RollSpawn(250.0f, 0, 6, false, 99.99f) == RollResult::Wild);   // clamped to 100
    // Cap first, then cooldown, whatever the roll.
    UMP_CHECK(RollSpawn(100.0f, 6, 6, false, 0.0f) == RollResult::ZoneFull);
    UMP_CHECK(RollSpawn(100.0f, 5, 6, true, 0.0f) == RollResult::Cooldown);
    UMP_CHECK(RollSpawn(100.0f, 6, 6, true, 0.0f) == RollResult::ZoneFull);
    UMP_CHECK(RollSpawn(100.0f, 99, 0, false, 0.0f) == RollResult::Wild);     // cap 0 = none

    // Cooldowns end on time and prune away.
    Cooldowns cd;
    cd.Start(42, 1000, 900);
    UMP_CHECK(cd.Active(42, 1000));
    UMP_CHECK(cd.Active(42, 1899));
    UMP_CHECK(!cd.Active(42, 1900));
    UMP_CHECK(cd.Remaining(42, 1500) == 400);
    UMP_CHECK(cd.Remaining(7, 1500) == 0);
    UMP_CHECK(!cd.Active(7, 1000));
    cd.Start(43, 1000, 5000);
    cd.Prune(2000);
    UMP_CHECK(cd.Size() == 1);
    UMP_CHECK(cd.Active(43, 2000));
    cd.Start(44, 1000, -5);   // negative cooldowns are none
    UMP_CHECK(!cd.Active(44, 1000));

    // Zone tally counts per (map, zone) and never goes below zero.
    ZoneTally tally;
    tally.Add(0, 12);
    tally.Add(0, 12);
    tally.Add(1, 12);
    UMP_CHECK(tally.Live(0, 12) == 2);
    UMP_CHECK(tally.Live(1, 12) == 1);
    UMP_CHECK(tally.Live(0, 14) == 0);
    UMP_CHECK(tally.Total() == 3);
    tally.Remove(0, 12);
    tally.Remove(0, 12);
    tally.Remove(0, 12);
    UMP_CHECK(tally.Live(0, 12) == 0);
    UMP_CHECK(tally.All().size() == 1);
    tally.Remove(5, 5);
    UMP_CHECK(tally.Total() == 1);

    // Health bonus spans [min, max] and tolerates swapped bounds.
    UMP_CHECK(HealthBonusPct(50, 100, 0.0f) == 50);
    UMP_CHECK(HealthBonusPct(50, 100, 1.0f) == 100);
    UMP_CHECK(HealthBonusPct(50, 100, 0.5f) == 75);
    UMP_CHECK(HealthBonusPct(100, 50, 0.0f) == 50);
    UMP_CHECK(HealthBonusPct(50, 100, 7.0f) == 100);

    // XP: base plus per affix, capped.
    UMP_CHECK(std::fabs(XpFraction(0.10f, 0.05f, 1, 0.20f) - 0.15f) < 1e-6f);
    UMP_CHECK(std::fabs(XpFraction(0.10f, 0.05f, 2, 0.20f) - 0.20f) < 1e-6f);
    UMP_CHECK(std::fabs(XpFraction(0.10f, 0.05f, 5, 0.20f) - 0.20f) < 1e-6f);
    UMP_CHECK(XpFraction(0.10f, 0.05f, 1, -1.0f) == 0.0f);
}
