"""Pure scaling, comparator matching, and stat-allocation math for twink tuning (T-050).

Implements the server's heirloom curves (`ScalingStatDistribution.dbc`,
`ScalingStatValues.dbc`, `ItemDamage*.dbc`) and item stat allocation rules
(`RandPropPoints.dbc`, `gtItemSocketCostPerLevel.dbc`) documented in
`docs/twinks.md` (T-045).
"""

from __future__ import annotations

import math
import struct
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Iterable, List, Mapping, Optional, Sequence, Tuple

# ItemModType constants from ItemPrototype.h
ITEM_MOD_AGILITY = 3
ITEM_MOD_STRENGTH = 4
ITEM_MOD_INTELLECT = 5
ITEM_MOD_SPIRIT = 6
ITEM_MOD_STAMINA = 7
ITEM_MOD_HIT_RATING = 31
ITEM_MOD_CRIT_RATING = 32
ITEM_MOD_HASTE_RATING = 36
ITEM_MOD_EXPERTISE_RATING = 37
ITEM_MOD_ATTACK_POWER = 38
ITEM_MOD_RANGED_ATTACK_POWER = 39
ITEM_MOD_SPELL_POWER = 45
ITEM_MOD_MASTERY_RATING = 49

PRIMARY_STATS = frozenset({ITEM_MOD_AGILITY, ITEM_MOD_STRENGTH, ITEM_MOD_INTELLECT})
OFFENSE_STATS = frozenset({
    ITEM_MOD_AGILITY,
    ITEM_MOD_STRENGTH,
    ITEM_MOD_INTELLECT,
    ITEM_MOD_HIT_RATING,
    ITEM_MOD_CRIT_RATING,
    ITEM_MOD_HASTE_RATING,
    ITEM_MOD_EXPERTISE_RATING,
    ITEM_MOD_ATTACK_POWER,
    ITEM_MOD_RANGED_ATTACK_POWER,
    ITEM_MOD_SPELL_POWER,
    ITEM_MOD_MASTERY_RATING,
})

ITEM_FLAGS_EXTRA_CASTER_WEAPON = 0x200
DEFAULT_BRACKETS: Tuple[int, ...] = (19, 29, 39, 49, 59, 60, 69, 70, 78, 79, 80, 84, 85, 89)
DEFAULT_TARGET_PCT: float = 7.0
ITEM_SPARSE_HOTFIX_TYPE: int = 2442913102


@dataclass(frozen=True)
class StatWeights:
    """Heuristic stat weights used to rank matching heirloom comparators."""

    primary: float = 1.0
    stamina: float = 0.25
    crit: float = 1.25
    haste: float = 1.25
    hit: float = 1.0
    expertise: float = 1.0
    attack_power: float = 0.5
    ranged_attack_power: float = 0.5
    spell_power: float = 1.0
    mastery: float = 1.0

    def weight_for(self, stat_type: int) -> float:
        if stat_type in PRIMARY_STATS:
            return self.primary
        if stat_type == ITEM_MOD_STAMINA:
            return self.stamina
        if stat_type == ITEM_MOD_CRIT_RATING:
            return self.crit
        if stat_type == ITEM_MOD_HASTE_RATING:
            return self.haste
        if stat_type == ITEM_MOD_HIT_RATING:
            return self.hit
        if stat_type == ITEM_MOD_EXPERTISE_RATING:
            return self.expertise
        if stat_type == ITEM_MOD_ATTACK_POWER:
            return self.attack_power
        if stat_type == ITEM_MOD_RANGED_ATTACK_POWER:
            return self.ranged_attack_power
        if stat_type == ITEM_MOD_SPELL_POWER:
            return self.spell_power
        if stat_type == ITEM_MOD_MASTERY_RATING:
            return self.mastery
        return 0.0

    @classmethod
    def from_string(cls, spec: str) -> "StatWeights":
        """Parse comma-separated `key=value` weight overrides."""
        if not spec or not spec.strip():
            return cls()
        kwargs: Dict[str, float] = {}
        aliases = {
            "primary": "primary",
            "agi": "primary",
            "str": "primary",
            "int": "primary",
            "stamina": "stamina",
            "sta": "stamina",
            "crit": "crit",
            "haste": "haste",
            "hit": "hit",
            "expertise": "expertise",
            "exp": "expertise",
            "attack_power": "attack_power",
            "ap": "attack_power",
            "ranged_attack_power": "ranged_attack_power",
            "rap": "ranged_attack_power",
            "spell_power": "spell_power",
            "sp": "spell_power",
            "mastery": "mastery",
        }
        for part in spec.split(","):
            part = part.strip()
            if not part:
                continue
            if "=" not in part:
                raise ValueError(f"Invalid weight specification '{part}' (expected key=value)")
            raw_k, raw_v = part.split("=", 1)
            k = raw_k.strip().lower()
            if k not in aliases:
                raise ValueError(f"Unknown stat weight key '{raw_k}'")
            target_field = aliases[k]
            val = float(raw_v.strip())
            kwargs[target_field] = val
            if target_field == "attack_power" and "ranged_attack_power" not in kwargs:
                kwargs["ranged_attack_power"] = val
        return cls(**kwargs)


@dataclass
class StatSlot:
    """Single stat slot (`stat_typeN`, `stat_valueN`, `stat_unk1_N`, `stat_unk2_N`)."""

    stat_type: int
    stat_value: int
    stat_alloc: int
    socket_mult: float


@dataclass
class ItemRecord:
    """Item template fields needed for twink comparator matching and buff calculation."""

    entry: int
    name: str
    item_class: int
    subclass: int
    inventory_type: int
    quality: int
    item_level: int
    flags_extra: int
    random_property: int = 0
    random_suffix: int = 0
    scaling_stat_distribution: int = 0
    stats: List[StatSlot] = field(default_factory=list)

    @property
    def is_caster_weapon(self) -> bool:
        return bool(self.flags_extra & ITEM_FLAGS_EXTRA_CASTER_WEAPON)

    @property
    def primary_stat_types(self) -> frozenset[int]:
        return frozenset(
            s.stat_type for s in self.stats if s.stat_type in PRIMARY_STATS and s.stat_value > 0
        )


@dataclass(frozen=True)
class ItemUpdate:
    """Generated SQL update for a single shortlisted candidate item."""

    bracket: int
    entry: int
    name: str
    comparator_entry: int
    comparator_name: str
    old_ilvl: int
    new_ilvl: int
    set_clauses: Tuple[str, ...]

    @property
    def comment_line(self) -> str:
        clean_comp = self.comparator_name.replace('"', "")
        return f"-- {self.bracket}: {self.name}; comparator {self.comparator_entry} {clean_comp}"

    @property
    def update_line(self) -> str:
        return f"UPDATE item_template SET {', '.join(self.set_clauses)} WHERE entry={self.entry};"


def normalize_slot(inventory_type: int) -> int:
    """Robes (20) and chests (5) share the chest slot for heirloom matching."""
    return 5 if inventory_type == 20 else inventory_type


def ssv_stat_multiplier(ssv_row: Sequence[int], inventory_type: int) -> int:
    """Port of `ScalingStatValuesEntry::GetStatMultiplier` (`DBCStores.cpp`)."""
    if inventory_type in (1, 5, 7, 17, 20):
        return int(ssv_row[10])
    if inventory_type in (3, 6, 8, 10, 12):
        return int(ssv_row[11])
    if inventory_type in (2, 9, 11, 14, 16, 23):
        return int(ssv_row[12])
    if inventory_type in (15, 25, 26, 28):
        return int(ssv_row[13])
    if inventory_type in (13, 21, 22):
        return int(ssv_row[14])
    return 0


def ssv_weapon_dps(ssv_row: Sequence[int], subclass: int, is_caster: bool) -> int:
    """Port of `ScalingStatValuesEntry::GetDPSAndDamageMultiplier` (`DBCStores.cpp`)."""
    if not is_caster:
        if subclass in (0, 4, 7, 15, 16):
            return int(ssv_row[2 + 0])
        if subclass in (1, 5, 6, 8, 10, 20):
            return int(ssv_row[2 + 1])
        if subclass in (2, 3, 18):
            return int(ssv_row[2 + 4])
        if subclass == 13:
            return int(ssv_row[2 + 1])
    else:
        if subclass <= 19:
            mask = 1 << subclass
            if mask & 0x562:
                return int(ssv_row[2 + 3])
            if mask & (1 << 19):
                return int(ssv_row[2 + 5])
        return int(ssv_row[2 + 2])
    return 0


def effective_heirloom_stats(
    heirloom: ItemRecord,
    endpoint_level: int,
    ssd_store: Mapping[int, Sequence[int]],
    ssv_store: Mapping[int, Sequence[int]],
) -> Tuple[List[Tuple[int, int]], int]:
    """Compute effective `(stat_type, value)` pairs and weapon DPS for an heirloom at `endpoint_level`."""
    ssd = ssd_store.get(heirloom.scaling_stat_distribution)
    if not ssd:
        return [], 0
    max_level = int(ssd[22])
    eff_level = min(endpoint_level, max_level)
    ssv = ssv_store.get(eff_level)
    if not ssv:
        return [], 0

    mult = ssv_stat_multiplier(ssv, heirloom.inventory_type)
    stats: List[Tuple[int, int]] = []
    for i in range(10):
        stat_type = int(ssd[1 + i])
        modifier = int(ssd[11 + i])
        if stat_type >= 0 and modifier > 0:
            val = (mult * modifier) // 10000
            if val > 0:
                stats.append((stat_type, val))

    # Caster weapons receive additional spell power from ScalingStatValues.dbc
    if heirloom.is_caster_weapon and int(ssv[9]) > 0:
        sp = int(ssv[9])
        for idx, (st, val) in enumerate(stats):
            if st == ITEM_MOD_SPELL_POWER:
                stats[idx] = (ITEM_MOD_SPELL_POWER, val + sp)
                break
        else:
            stats.append((ITEM_MOD_SPELL_POWER, sp))

    dps = 0
    if heirloom.item_class == 2:
        dps = ssv_weapon_dps(ssv, heirloom.subclass, heirloom.is_caster_weapon)
    return stats, dps


def score_heirloom_stats(stats: Iterable[Tuple[int, int]], weights: StatWeights) -> float:
    """Compute the DPS-weighted stat score used to rank matching heirloom comparators."""
    return sum(val * weights.weight_for(st) for st, val in stats)


def choose_comparator(
    candidate: ItemRecord,
    heirlooms: Sequence[ItemRecord],
    endpoint_level: int,
    ssd_store: Mapping[int, Sequence[int]],
    ssv_store: Mapping[int, Sequence[int]],
    weights: StatWeights,
) -> Optional[Tuple[ItemRecord, List[Tuple[int, int]], int, float]]:
    """Select the highest-scoring matching heirloom comparator at `endpoint_level`."""
    cand_prim = candidate.primary_stat_types
    if not cand_prim:
        return None
    cand_slot = normalize_slot(candidate.inventory_type)
    cand_caster = candidate.is_caster_weapon

    matches: List[Tuple[float, int, ItemRecord, List[Tuple[int, int]], int]] = []
    for h in heirlooms:
        if h.item_class != candidate.item_class or h.subclass != candidate.subclass:
            continue
        if normalize_slot(h.inventory_type) != cand_slot:
            continue
        if h.is_caster_weapon != cand_caster:
            continue
        h_stats, h_dps = effective_heirloom_stats(h, endpoint_level, ssd_store, ssv_store)
        h_prim = frozenset(st for st, val in h_stats if st in PRIMARY_STATS and val > 0)
        if h_prim != cand_prim:
            continue
        score = score_heirloom_stats(h_stats, weights)
        matches.append((score, -h.entry, h, h_stats, h_dps))

    if not matches:
        return None
    matches.sort(reverse=True)
    best_score, _, best_h, best_stats, best_dps = matches[0]
    return best_h, best_stats, best_dps, best_score


def get_random_points_offset(inventory_type: int, subclass: int) -> int:
    """Return the `RandPropPoints.dbc` column offset (`0..4`) for an inventory type."""
    if inventory_type in (1, 4, 5, 7, 15, 17, 20, 25):
        return 0
    if inventory_type in (13, 21, 22, 26):
        return 3
    if inventory_type in (3, 6, 8, 10, 12):
        return 1
    if inventory_type in (2, 9, 11, 14, 16, 23):
        return 2
    if inventory_type == 28:
        return 4
    return -1


def calculate_scaling_stat_value(
    ilvl: int,
    quality: int,
    inventory_type: int,
    subclass: int,
    rpp_store: Mapping[int, Sequence[int]],
) -> int:
    """Port of `ItemTemplate::CalculateScalingStatValue` (`Item.cpp`)."""
    offset = get_random_points_offset(inventory_type, subclass)
    if offset == -1:
        return 0
    rpp = rpp_store.get(ilvl)
    if not rpp:
        return 0
    if quality == 2:
        return int(rpp[11 + offset])
    if quality == 3:
        return int(rpp[6 + offset])
    if quality in (4, 5, 6):
        return int(rpp[1 + offset])
    return 0


def calculate_stat_allocation(
    stat_value: int,
    ilvl: int,
    quality: int,
    inventory_type: int,
    subclass: int,
    socket_mult: float,
    rpp_store: Mapping[int, Sequence[int]],
    gt_socket_store: Mapping[int, Sequence[float]],
) -> int:
    """Compute `stat_unk1_N` (`ItemStatAllocation`) so `Item::GetStatValue` reproduces `stat_value`."""
    rp = calculate_scaling_stat_value(ilvl, quality, inventory_type, subclass, rpp_store)
    if rp <= 0:
        return 0
    sock_entry = gt_socket_store.get(ilvl)
    sock_cost = float(sock_entry[1]) if sock_entry else 0.0
    penalty = int(socket_mult * sock_cost)
    return math.ceil((stat_value + penalty) * 10000 / rp)


def compute_target_stat(heirloom_stat_value: int, target_pct: float) -> int:
    """Apply the target percentage floor (`ceil((1 + target_pct/100) * heirloom_stat_value)`)."""
    mult = 1.0 + (target_pct / 100.0)
    return math.ceil(mult * heirloom_stat_value - 1e-9)


def get_damage_store_name(inventory_type: int, subclass: int, is_caster: bool) -> Optional[str]:
    """Map weapon inventory type, subclass, and caster flag to its `ItemDamage*.dbc` store name."""
    if inventory_type == 24:
        return "ItemDamageAmmo"
    if inventory_type == 17:
        return "ItemDamageTwoHandCaster" if is_caster else "ItemDamageTwoHand"
    if inventory_type in (15, 25, 26):
        if subclass == 19:
            return "ItemDamageWand"
        if subclass == 16:
            return "ItemDamageThrown"
        if subclass in (2, 3, 18):
            return "ItemDamageRanged"
        return None
    if inventory_type in (13, 21, 22):
        return "ItemDamageOneHandCaster" if is_caster else "ItemDamageOneHand"
    return None


def get_weapon_dps(
    ilvl: int,
    quality: int,
    inventory_type: int,
    subclass: int,
    is_caster: bool,
    damage_stores: Mapping[str, Mapping[int, Sequence[float]]],
) -> float:
    """Look up weapon DPS from `ItemDamage*.dbc` at `ilvl` and `quality`."""
    store_name = get_damage_store_name(inventory_type, subclass, is_caster)
    if not store_name:
        return 0.0
    store = damage_stores.get(store_name)
    if not store:
        return 0.0
    row = store.get(ilvl)
    if not row:
        return 0.0
    q = 3 if quality == 7 else quality
    if q < 0 or q > 6:
        return 0.0
    return float(row[1 + q])


def find_minimum_weapon_ilvl(
    current_ilvl: int,
    quality: int,
    inventory_type: int,
    subclass: int,
    is_caster: bool,
    heirloom_dps: float,
    target_pct: float,
    damage_stores: Mapping[str, Mapping[int, Sequence[float]]],
) -> int:
    """Find the smallest item level `>= current_ilvl` whose weapon DPS meets `(1 + target_pct/100) * heirloom_dps`."""
    if heirloom_dps <= 0:
        return current_ilvl
    target_dps = (1.0 + target_pct / 100.0) * heirloom_dps
    ilvl = current_ilvl
    while ilvl < 1000:
        cur_dps = get_weapon_dps(ilvl, quality, inventory_type, subclass, is_caster, damage_stores)
        if cur_dps >= target_dps or cur_dps == 0.0:
            break
        ilvl += 1
    return ilvl


def compute_item_buff(
    candidate: ItemRecord,
    comparator: ItemRecord,
    h_stats: Sequence[Tuple[int, int]],
    h_dps: int,
    endpoint_level: int,
    target_pct: float,
    rpp_store: Mapping[int, Sequence[int]],
    gt_socket_store: Mapping[int, Sequence[float]],
    damage_stores: Mapping[str, Mapping[int, Sequence[float]]],
) -> Optional[ItemUpdate]:
    """Compute the stat/itemLevel floor updates for a candidate item against its chosen comparator."""
    if candidate.random_property != 0 or candidate.random_suffix != 0:
        return None

    new_ilvl = candidate.item_level
    if candidate.item_class == 2 and h_dps > 0:
        new_ilvl = find_minimum_weapon_ilvl(
            candidate.item_level,
            candidate.quality,
            candidate.inventory_type,
            candidate.subclass,
            candidate.is_caster_weapon,
            h_dps,
            target_pct,
            damage_stores,
        )

    cur_stats = [
        StatSlot(s.stat_type, s.stat_value, s.stat_alloc, s.socket_mult)
        for s in candidate.stats
    ]
    set_clauses: List[str] = []
    modified_slots = set()

    for st, h_val in h_stats:
        if st not in OFFENSE_STATS or h_val <= 0:
            continue
        floor_val = compute_target_stat(h_val, target_pct)
        slot_idx = next(
            (i for i, s in enumerate(cur_stats) if s.stat_type == st and s.stat_value > 0),
            None,
        )
        if slot_idx is not None:
            if cur_stats[slot_idx].stat_value < floor_val:
                cur_stats[slot_idx].stat_value = floor_val
                uk = calculate_stat_allocation(
                    floor_val,
                    new_ilvl,
                    candidate.quality,
                    candidate.inventory_type,
                    candidate.subclass,
                    cur_stats[slot_idx].socket_mult,
                    rpp_store,
                    gt_socket_store,
                )
                cur_stats[slot_idx].stat_alloc = uk
                set_clauses.append(f"stat_value{slot_idx + 1}={floor_val}")
                set_clauses.append(f"stat_unk1_{slot_idx + 1}={uk}")
                modified_slots.add(slot_idx)
        else:
            empty_idx = next(
                (i for i, s in enumerate(cur_stats) if s.stat_type <= 0 or s.stat_value <= 0),
                None,
            )
            if empty_idx is not None:
                cur_stats[empty_idx].stat_type = st
                cur_stats[empty_idx].stat_value = floor_val
                uk = calculate_stat_allocation(
                    floor_val,
                    new_ilvl,
                    candidate.quality,
                    candidate.inventory_type,
                    candidate.subclass,
                    cur_stats[empty_idx].socket_mult,
                    rpp_store,
                    gt_socket_store,
                )
                cur_stats[empty_idx].stat_alloc = uk
                set_clauses.append(f"stat_type{empty_idx + 1}={st}")
                set_clauses.append(f"stat_value{empty_idx + 1}={floor_val}")
                set_clauses.append(f"stat_unk1_{empty_idx + 1}={uk}")
                modified_slots.add(empty_idx)

    if new_ilvl != candidate.item_level:
        set_clauses.append(f"ItemLevel={new_ilvl}")
        for i, s in enumerate(cur_stats):
            if i not in modified_slots and s.stat_type > 0 and s.stat_value > 0:
                uk = calculate_stat_allocation(
                    s.stat_value,
                    new_ilvl,
                    candidate.quality,
                    candidate.inventory_type,
                    candidate.subclass,
                    s.socket_mult,
                    rpp_store,
                    gt_socket_store,
                )
                if uk != s.stat_alloc:
                    set_clauses.append(f"stat_unk1_{i + 1}={uk}")

    if not set_clauses:
        return None

    return ItemUpdate(
        bracket=endpoint_level,
        entry=candidate.entry,
        name=candidate.name,
        comparator_entry=comparator.entry,
        comparator_name=comparator.name,
        old_ilvl=candidate.item_level,
        new_ilvl=new_ilvl,
        set_clauses=tuple(set_clauses),
    )


def read_dbc_file(path: Path, fmt: str) -> Dict[int, List]:
    """Read a fixed-layout WDBC file using a TrinityCore `DBCfmt.h` format string."""
    data = path.read_bytes()
    if len(data) < 20 or data[:4] != b"WDBC":
        raise ValueError(f"Expected WDBC header in {path}")
    rec_count, _field_count, rec_size, _str_size = struct.unpack_from("<4I", data, 4)
    header_size = 20
    s_fmt = "<" + "".join("i" if c in ("n", "i", "x") else ("f" if c == "f" else "I") for c in fmt)
    unpacker = struct.Struct(s_fmt)
    key_idx = fmt.index("n") if "n" in fmt else 0
    records: Dict[int, List] = {}
    for idx in range(rec_count):
        off = header_size + idx * rec_size
        vals = list(unpacker.unpack_from(data, off))
        records[int(vals[key_idx])] = vals
    return records


def load_scaling_dbcs(dbc_dir: Path) -> Tuple[
    Dict[int, List],
    Dict[int, List],
    Dict[int, List],
    Dict[int, List],
    Dict[str, Dict[int, List]],
]:
    """Load all DBC stores required for heirloom scaling and item stat allocation."""
    ssd_store = read_dbc_file(dbc_dir / "ScalingStatDistribution.dbc", "niiiiiiiiiiiiiiiiiiiixi")
    ssv_store = read_dbc_file(
        dbc_dir / "ScalingStatValues.dbc",
        "iniiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiiii",
    )
    rpp_store = read_dbc_file(dbc_dir / "RandPropPoints.dbc", "niiiiiiiiiiiiiii")
    gt_socket = read_dbc_file(dbc_dir / "gtItemSocketCostPerLevel.dbc", "nf")
    damage_stores = {
        name: read_dbc_file(dbc_dir / f"{name}.dbc", "nfffffffi")
        for name in (
            "ItemDamageAmmo",
            "ItemDamageOneHand",
            "ItemDamageOneHandCaster",
            "ItemDamageRanged",
            "ItemDamageThrown",
            "ItemDamageTwoHand",
            "ItemDamageTwoHandCaster",
            "ItemDamageWand",
        )
    }
    return ssd_store, ssv_store, rpp_store, gt_socket, damage_stores
