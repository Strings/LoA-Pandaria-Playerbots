"""Unit tests for pure math in tools/twink-tuning (T-050)."""

from __future__ import annotations

import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import tuning_math as tm
import twink_tuning as tt


def _make_ssd_row(ssd_id: int, max_level: int, pairs: list[tuple[int, int]]) -> list[int]:
    stat_mods = [-1] * 10
    modifiers = [0] * 10
    for i, (st, mod) in enumerate(pairs[:10]):
        stat_mods[i] = st
        modifiers[i] = mod
    # [Id, StatMod[0..9], Modifier[0..9], unk1, MaxLevel]
    return [ssd_id, *stat_mods, *modifiers, 0, max_level]


def _make_ssv_row(
    level: int,
    *,
    dps_mods: tuple[int, ...] = (10, 20, 15, 25, 18, 12, 0),
    spellpower: int = 0,
    stat_mults: tuple[int, int, int, int, int] = (100, 75, 56, 30, 42),
) -> list[int]:
    # [Id, Level, dpsMod[0..6], Spellpower, StatMultiplier[0..4], Armor[32], CloakArmor, unk]
    return [level, level, *dps_mods, spellpower, *stat_mults, *([0] * 34)]


class TestTwinkTuningMath(unittest.TestCase):
    def test_stat_weights_parsing_and_lookup(self) -> None:
        default_w = tm.StatWeights()
        self.assertEqual(default_w.weight_for(tm.ITEM_MOD_AGILITY), 1.0)
        self.assertEqual(default_w.weight_for(tm.ITEM_MOD_STRENGTH), 1.0)
        self.assertEqual(default_w.weight_for(tm.ITEM_MOD_INTELLECT), 1.0)
        self.assertEqual(default_w.weight_for(tm.ITEM_MOD_STAMINA), 0.25)
        self.assertEqual(default_w.weight_for(tm.ITEM_MOD_CRIT_RATING), 1.25)
        self.assertEqual(default_w.weight_for(tm.ITEM_MOD_HASTE_RATING), 1.25)
        self.assertEqual(default_w.weight_for(tm.ITEM_MOD_ATTACK_POWER), 0.5)
        self.assertEqual(default_w.weight_for(tm.ITEM_MOD_SPIRIT), 0.0)

        custom_w = tm.StatWeights.from_string("stamina=0.5,crit=1.5,haste=1.4,ap=0.6")
        self.assertEqual(custom_w.stamina, 0.5)
        self.assertEqual(custom_w.crit, 1.5)
        self.assertEqual(custom_w.haste, 1.4)
        self.assertEqual(custom_w.attack_power, 0.6)
        self.assertEqual(custom_w.ranged_attack_power, 0.6)

        with self.assertRaises(ValueError):
            tm.StatWeights.from_string("unknown_stat=1.0")

    def test_stat_budget_per_bracket_and_max_level_cap(self) -> None:
        ssd_store = {
            5: _make_ssd_row(
                5,
                max_level=80,
                pairs=[
                    (tm.ITEM_MOD_INTELLECT, 8900),
                    (tm.ITEM_MOD_STAMINA, 10000),
                    (tm.ITEM_MOD_CRIT_RATING, 5933),
                ],
            )
        }
        ssv_store = {
            19: _make_ssv_row(19, spellpower=39, stat_mults=(8, 6, 4, 2, 3)),
            80: _make_ssv_row(80, spellpower=450, stat_mults=(100, 75, 56, 30, 42)),
            85: _make_ssv_row(85, spellpower=900, stat_mults=(200, 150, 112, 60, 84)),
        }
        caster_staff = tm.ItemRecord(
            entry=42947,
            name="Dignified Headmaster's Charge",
            item_class=2,
            subclass=10,
            inventory_type=17,
            quality=7,
            item_level=1,
            flags_extra=tm.ITEM_FLAGS_EXTRA_CASTER_WEAPON,
            scaling_stat_distribution=5,
        )

        # At level 19: StatMultiplier[0]=8 -> Int=(8*8900)//10000=7, Sta=8, Crit=4, SP=39
        stats_19, dps_19 = tm.effective_heirloom_stats(caster_staff, 19, ssd_store, ssv_store)
        self.assertEqual(
            dict(stats_19),
            {
                tm.ITEM_MOD_INTELLECT: 7,
                tm.ITEM_MOD_STAMINA: 8,
                tm.ITEM_MOD_CRIT_RATING: 4,
                tm.ITEM_MOD_SPELL_POWER: 39,
            },
        )
        self.assertEqual(dps_19, 25)  # 2H caster uses dpsMod[3]

        # At level 85: capped at ssd.MaxLevel=80
        stats_85, _ = tm.effective_heirloom_stats(caster_staff, 85, ssd_store, ssv_store)
        stats_80, _ = tm.effective_heirloom_stats(caster_staff, 80, ssd_store, ssv_store)
        self.assertEqual(stats_85, stats_80)

    def test_rounding_and_allocation_rebasing(self) -> None:
        # 7% above 7 -> 7 * 1.07 = 7.49 -> ceil = 8
        self.assertEqual(tm.compute_target_stat(7, 7.0), 8)
        # 7% above 39 -> 39 * 1.07 = 41.73 -> ceil = 42
        self.assertEqual(tm.compute_target_stat(39, 7.0), 42)
        # 0% target keeps exact integer value
        self.assertEqual(tm.compute_target_stat(10, 0.0), 10)

        # Shadowfang (1482): Rare (quality=3) 1H sword (inv=13 -> offset=3) at ilvl=38 -> randomPoints=9
        rpp_store = {
            38: [38, 15, 15, 15, 15, 15, 22, 16, 12, 9, 6, 11, 8, 6, 4, 3],
        }
        gt_socket = {38: [38, 3.0]}
        self.assertEqual(
            tm.calculate_stat_allocation(6, 38, 3, 13, 7, 0.0, rpp_store, gt_socket),
            6667,
        )
        self.assertEqual(
            tm.calculate_stat_allocation(4, 38, 3, 13, 7, 0.0, rpp_store, gt_socket),
            4445,
        )
        # With socket cost multiplier (e.g. socket_mult=1.0 -> penalty=3 -> (6+3)*10000/9 = 10000)
        self.assertEqual(
            tm.calculate_stat_allocation(6, 38, 3, 13, 7, 1.0, rpp_store, gt_socket),
            10000,
        )

    def test_comparator_selection_and_item_buff(self) -> None:
        ssd_store = {
            10: _make_ssd_row(
                10,
                max_level=80,
                pairs=[
                    (tm.ITEM_MOD_AGILITY, 10000),
                    (tm.ITEM_MOD_STAMINA, 10000),
                    (tm.ITEM_MOD_CRIT_RATING, 5000),
                ],
            ),
            11: _make_ssd_row(
                11,
                max_level=80,
                pairs=[
                    (tm.ITEM_MOD_AGILITY, 10000),
                    (tm.ITEM_MOD_STAMINA, 10000),
                    (tm.ITEM_MOD_HIT_RATING, 5000),
                ],
            ),
        }
        ssv_store = {19: _make_ssv_row(19, stat_mults=(10, 10, 10, 10, 10))}
        h_crit = tm.ItemRecord(
            entry=1001,
            name="Crit Chest",
            item_class=4,
            subclass=2,
            inventory_type=5,
            quality=7,
            item_level=1,
            flags_extra=0,
            scaling_stat_distribution=10,
        )
        h_hit = tm.ItemRecord(
            entry=1002,
            name="Hit Chest",
            item_class=4,
            subclass=2,
            inventory_type=5,
            quality=7,
            item_level=1,
            flags_extra=0,
            scaling_stat_distribution=11,
        )
        # Candidate is a robe (inv_type=20, shares slot 5) with Agi + Sta and an empty slot 2 (val=0)
        cand = tm.ItemRecord(
            entry=2001,
            name="Twink Leather Robe",
            item_class=4,
            subclass=2,
            inventory_type=20,
            quality=3,
            item_level=25,
            flags_extra=0,
            stats=[
                tm.StatSlot(tm.ITEM_MOD_STAMINA, 15, 5000, 0.0),
                tm.StatSlot(tm.ITEM_MOD_CRIT_RATING, 0, 0, 0.0),
                tm.StatSlot(tm.ITEM_MOD_AGILITY, 8, 4000, 0.0),
                *[tm.StatSlot(0, 0, 0, 0.0) for _ in range(7)],
            ],
        )
        weights = tm.StatWeights()
        match = tm.choose_comparator(cand, [h_hit, h_crit], 19, ssd_store, ssv_store, weights)
        self.assertIsNotNone(match)
        assert match is not None
        comp, h_stats, h_dps, _ = match
        # Crit (weight 1.25) beats Hit (weight 1.0)
        self.assertEqual(comp.entry, 1001)

        rpp_store = {25: [25, *([20] * 15)]}
        gt_socket = {25: [25, 0.0]}
        upd = tm.compute_item_buff(
            cand, comp, h_stats, h_dps, 19, 7.0, rpp_store, gt_socket, {}
        )
        self.assertIsNotNone(upd)
        assert upd is not None
        # Agi 8 -> ceil(10 * 1.07)=11 in slot 3; Crit 0 -> ceil(5 * 1.07)=6 in empty slot 2; Stamina 15 untouched
        self.assertEqual(
            upd.set_clauses,
            (
                "stat_value3=11",
                "stat_unk1_3=5500",
                "stat_type2=32",
                "stat_value2=6",
                "stat_unk1_2=3000",
            ),
        )

    def test_weapon_ilvl_floor_and_existing_stat_rebasing(self) -> None:
        # Shadowfang (1482): Rare (quality=3) 1H sword (inv=13, sub=7), old ilvl=24 -> new ilvl=38
        damage_stores = {
            "ItemDamageOneHand": {
                ilvl: [ilvl, 0.0, 0.0, 0.0, 10.0 + (ilvl - 24) * 0.5, 0.0, 0.0, 0.0, ilvl]
                for ilvl in range(24, 45)
            }
        }
        # Heirloom DPS = 15.88 -> target = 1.07 * 15.88 = 16.99 -> needs DPS >= 17.0 -> ilvl = 38
        new_ilvl = tm.find_minimum_weapon_ilvl(24, 3, 13, 7, False, 15.88, 7.0, damage_stores)
        self.assertEqual(new_ilvl, 38)

    def test_parse_brackets(self) -> None:
        self.assertEqual(tt.parse_brackets("19, 29, 39, 19"), [19, 29, 39])
        with self.assertRaises(ValueError):
            tt.parse_brackets("0,19")
        with self.assertRaises(ValueError):
            tt.parse_brackets("")


if __name__ == "__main__":
    unittest.main()
