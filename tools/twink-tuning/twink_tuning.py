#!/usr/bin/env python3
"""Regenerate twink stat-buff SQL against a read-only MoP world database (T-050).

Replaces `sql/updates/world/2026_10_31_codex_00_twink_gear.sql` using the method
and numbers in `docs/twinks.md` (T-045), parameterized by target percentage,
stat weights, and bracket endpoints.
"""

from __future__ import annotations

import argparse
import json
import os
import shutil
import subprocess
import sys
from pathlib import Path
from typing import Dict, List, Optional, Sequence, Tuple

try:
    from .tuning_math import (
        DEFAULT_BRACKETS,
        DEFAULT_TARGET_PCT,
        ITEM_SPARSE_HOTFIX_TYPE,
        ItemRecord,
        ItemUpdate,
        StatSlot,
        StatWeights,
        choose_comparator,
        compute_item_buff,
        load_scaling_dbcs,
    )
except ImportError:
    from tuning_math import (  # type: ignore[no-redef]
        DEFAULT_BRACKETS,
        DEFAULT_TARGET_PCT,
        ITEM_SPARSE_HOTFIX_TYPE,
        ItemRecord,
        ItemUpdate,
        StatSlot,
        StatWeights,
        choose_comparator,
        compute_item_buff,
        load_scaling_dbcs,
    )

NAME_FILTER_REGEX = r"(^QA|Deprecated|Test |PH\]|OLD|UNUSED)"
DEFAULT_HOTFIX_DATE = "2026-10-31 12:00:00"
BASELINE_PATH = Path(__file__).resolve().parent / "v104_baseline.json"


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def default_dbc_dir() -> Path:
    env_dir = os.environ.get("MOP_DBC_DIR")
    if env_dir:
        return Path(env_dir)
    root = repo_root()
    for candidate in (root / "gamedata" / "dbc", root.parent.parent / "gamedata" / "dbc"):
        if candidate.is_dir():
            return candidate
    return Path(r"I:\UltimateMoP\gamedata\dbc")


def find_mysql_binary(explicit: Optional[str] = None) -> str:
    candidates = [
        explicit,
        os.environ.get("MOP_MYSQL_BIN"),
        shutil.which("mysql"),
        r"C:\Program Files\MySQL\MySQL Server 8.0\bin\mysql.exe",
        r"C:\Program Files\MySQL\MySQL Server 8.4\bin\mysql.exe",
        r"C:\xampp\mysql\bin\mysql.exe",
    ]
    for candidate in candidates:
        if candidate and Path(candidate).exists():
            return str(candidate)
    raise SystemExit("mysql client not found; set MOP_MYSQL_BIN or pass --mysql-bin")


def parse_brackets(spec: str) -> List[int]:
    brackets: List[int] = []
    for raw in spec.split(","):
        part = raw.strip()
        if not part:
            continue
        level = int(part)
        if level < 1 or level > 90:
            raise ValueError(f"Bracket level out of range (1..90): {level}")
        if level not in brackets:
            brackets.append(level)
    if not brackets:
        raise ValueError("At least one bracket level is required")
    return brackets


def load_v104_baseline(path: Path = BASELINE_PATH) -> Dict[int, Dict]:
    if not path.is_file():
        return {}
    raw = json.loads(path.read_text(encoding="utf-8"))
    return {int(k): v for k, v in raw.items()}


def build_shortlist_sql(brackets: Sequence[int], baseline: Dict[int, Dict]) -> str:
    levels_sql = " UNION ALL ".join(f"SELECT {lvl} AS lvl" for lvl in brackets)
    if baseline:
        when_clauses = " ".join(
            f"WHEN {eid} THEN {int(data['ItemLevel'])}"
            for eid, data in sorted(baseline.items())
        )
        ilvl_expr = f"CASE i.entry {when_clauses} ELSE i.ItemLevel END"
    else:
        ilvl_expr = "i.ItemLevel"

    return f"""
WITH rewards AS (
    SELECT RewardItem1 AS item, MinLevel AS lvl FROM quest_template WHERE RewardItem1 > 0
    UNION ALL SELECT RewardItem2, MinLevel FROM quest_template WHERE RewardItem2 > 0
    UNION ALL SELECT RewardItem3, MinLevel FROM quest_template WHERE RewardItem3 > 0
    UNION ALL SELECT RewardItem4, MinLevel FROM quest_template WHERE RewardItem4 > 0
    UNION ALL SELECT RewardChoiceItemID1, MinLevel FROM quest_template WHERE RewardChoiceItemID1 > 0
    UNION ALL SELECT RewardChoiceItemID2, MinLevel FROM quest_template WHERE RewardChoiceItemID2 > 0
    UNION ALL SELECT RewardChoiceItemID3, MinLevel FROM quest_template WHERE RewardChoiceItemID3 > 0
    UNION ALL SELECT RewardChoiceItemID4, MinLevel FROM quest_template WHERE RewardChoiceItemID4 > 0
    UNION ALL SELECT RewardChoiceItemID5, MinLevel FROM quest_template WHERE RewardChoiceItemID5 > 0
    UNION ALL SELECT RewardChoiceItemID6, MinLevel FROM quest_template WHERE RewardChoiceItemID6 > 0
),
q AS (
    SELECT item, MIN(lvl) AS lvl
    FROM rewards
    GROUP BY item
),
sources AS (
    SELECT item FROM creature_loot_template WHERE item > 0
    UNION SELECT item FROM gameobject_loot_template WHERE item > 0
    UNION SELECT item FROM item_loot_template WHERE item > 0
    UNION SELECT item FROM reference_loot_template WHERE item > 0
    UNION SELECT item FROM fishing_loot_template WHERE item > 0
    UNION SELECT item FROM npc_vendor WHERE item > 0
),
levels AS (
    {levels_sql}
),
ranked AS (
    SELECT
        l.lvl AS bracket,
        i.entry,
        i.Class,
        i.SubClass,
        i.InventoryType,
        ROW_NUMBER() OVER (
            PARTITION BY l.lvl, i.Class, i.SubClass, i.InventoryType
            ORDER BY {ilvl_expr} DESC, i.Quality DESC, i.entry ASC
        ) AS rnk
    FROM item_template i
    CROSS JOIN levels l
    LEFT JOIN q ON q.item = i.entry
    WHERE i.Quality BETWEEN 2 AND 5
      AND i.InventoryType IN (1, 2, 3, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 20, 21, 22, 23, 25, 26)
      AND i.Class IN (2, 4)
      AND i.ScalingStatDistribution = 0
      AND i.RequiredLevel <= l.lvl
      AND (EXISTS (SELECT 1 FROM sources s WHERE s.item = i.entry) OR q.lvl <= l.lvl)
      AND (i.RequiredLevel > 0 OR q.lvl <= l.lvl)
      AND i.name NOT REGEXP '{NAME_FILTER_REGEX}'
)
SELECT bracket, entry
FROM ranked
WHERE rnk <= 2
ORDER BY bracket, Class, SubClass, InventoryType, rnk;
""".strip()


def run_query(
    sql: str,
    *,
    mysql_bin: str,
    host: str,
    port: int,
    user: str,
    password: str,
    database: str,
) -> List[List[str]]:
    env = os.environ.copy()
    if password:
        env["MYSQL_PWD"] = password
    cmd = [
        mysql_bin,
        "--batch",
        "--raw",
        "--skip-column-names",
        "--default-character-set=utf8mb4",
        "-h",
        host,
        "-P",
        str(port),
        "-u",
        user,
        database,
        "-e",
        sql,
    ]
    result = subprocess.run(cmd, env=env, text=True, capture_output=True, check=False)
    if result.returncode != 0:
        raise RuntimeError(result.stderr.strip() or f"mysql exited with {result.returncode}")
    return [line.split("\t") for line in result.stdout.splitlines() if line]


def parse_item_row(row: Sequence[str], baseline: Dict[int, Dict]) -> ItemRecord:
    entry = int(row[0])
    item_level = int(row[6])
    stats = [
        StatSlot(
            stat_type=int(row[11 + i * 4]),
            stat_value=int(row[12 + i * 4]),
            stat_alloc=int(row[13 + i * 4]),
            socket_mult=float(row[14 + i * 4]),
        )
        for i in range(10)
    ]
    if entry in baseline:
        base = baseline[entry]
        item_level = int(base["ItemLevel"])
        stats = [
            StatSlot(
                stat_type=int(s[0]),
                stat_value=int(s[1]),
                stat_alloc=int(s[2]),
                socket_mult=float(s[3]),
            )
            for s in base["stats"]
        ]
    return ItemRecord(
        entry=entry,
        name=row[1],
        item_class=int(row[2]),
        subclass=int(row[3]),
        inventory_type=int(row[4]),
        quality=int(row[5]),
        item_level=item_level,
        flags_extra=int(row[7]),
        random_property=int(row[8]),
        random_suffix=int(row[9]),
        scaling_stat_distribution=int(row[10]),
        stats=stats,
    )


def _fmt_num(val: float) -> str:
    if val == int(val):
        return str(int(val))
    s = f"{val:g}"
    if s.startswith("0."):
        return s[1:]
    return s


def render_sql(
    updates: Sequence[ItemUpdate],
    *,
    target_pct: float,
    weights: StatWeights,
    hotfix_date: str = DEFAULT_HOTFIX_DATE,
) -> str:
    mult_str = f"{1.0 + target_pct / 100.0:.2f}".rstrip("0").rstrip(".")
    pct_str = _fmt_num(target_pct)
    lines = [
        f"-- T-045: {pct_str}% DPS-weighted stat-budget target above matching heirlooms.",
        "-- Generated against v1.0.4. Absolute values: safe to reapply.",
        (
            f"-- Stamina weight {_fmt_num(weights.stamina)}; "
            f"primary/hit/expertise/mastery {_fmt_num(weights.primary)}; "
            f"crit/haste {_fmt_num(weights.crit)}; "
            f"attack power {_fmt_num(weights.attack_power)}."
        ),
        "-- Stamina is never increased. Missing matching-heirloom offense stats use empty slots; no proc/socket/requirements changes.",
        f"-- Weapon DPS floor: {mult_str} * same-subclass matching heirloom at first selected endpoint.",
        "-- See docs/twinks.md for limitations and per-item audit.",
        "START TRANSACTION;",
    ]
    for u in updates:
        lines.append(u.comment_line)
        lines.append(u.update_line)

    if updates:
        entries_csv = ",".join(str(u.entry) for u in updates)
        values_csv = ",".join(
            f"({u.entry},{ITEM_SPARSE_HOTFIX_TYPE},'{hotfix_date}')" for u in updates
        )
        lines.append(
            f"DELETE FROM hotfix_data WHERE type={ITEM_SPARSE_HOTFIX_TYPE} "
            f"AND hotfixDate='{hotfix_date}' AND entry IN ({entries_csv});"
        )
        lines.append(
            f"INSERT INTO hotfix_data (entry,type,hotfixDate) VALUES {values_csv};"
        )
    lines.append("COMMIT;")
    return "\n".join(lines) + "\n"


def generate_updates(
    *,
    mysql_bin: str,
    host: str,
    port: int,
    user: str,
    password: str,
    database: str,
    dbc_dir: Path,
    brackets: Sequence[int],
    target_pct: float,
    weights: StatWeights,
    use_baseline: bool = True,
) -> List[ItemUpdate]:
    ssd_store, ssv_store, rpp_store, gt_socket, damage_stores = load_scaling_dbcs(dbc_dir)
    baseline = load_v104_baseline() if use_baseline else {}

    shortlist_rows = run_query(
        build_shortlist_sql(brackets, baseline),
        mysql_bin=mysql_bin,
        host=host,
        port=port,
        user=user,
        password=password,
        database=database,
    )
    first_endpoint: List[Tuple[int, int]] = []
    seen = set()
    for b_str, e_str in shortlist_rows:
        b, e = int(b_str), int(e_str)
        if e not in seen:
            seen.add(e)
            first_endpoint.append((b, e))

    if not first_endpoint:
        return []

    stat_cols = ", ".join(
        f"stat_type{i}, stat_value{i}, stat_unk1_{i}, stat_unk2_{i}" for i in range(1, 11)
    )
    cand_ids_csv = ",".join(str(e) for _, e in first_endpoint)
    cand_rows = run_query(
        f"SELECT entry, name, Class, SubClass, InventoryType, Quality, ItemLevel, FlagsExtra, "
        f"RandomProperty, RandomSuffix, ScalingStatDistribution, {stat_cols} "
        f"FROM item_template WHERE entry IN ({cand_ids_csv})",
        mysql_bin=mysql_bin,
        host=host,
        port=port,
        user=user,
        password=password,
        database=database,
    )
    candidates = {int(r[0]): parse_item_row(r, baseline) for r in cand_rows}

    heirloom_rows = run_query(
        f"SELECT entry, name, Class, SubClass, InventoryType, Quality, ItemLevel, FlagsExtra, "
        f"RandomProperty, RandomSuffix, ScalingStatDistribution, {stat_cols} "
        f"FROM item_template WHERE Quality = 7 AND ScalingStatDistribution > 0",
        mysql_bin=mysql_bin,
        host=host,
        port=port,
        user=user,
        password=password,
        database=database,
    )
    heirlooms = [
        parse_item_row(r, {})
        for r in heirloom_rows
        if int(r[10]) in ssd_store
    ]

    updates: List[ItemUpdate] = []
    for bracket, entry in first_endpoint:
        cand = candidates.get(entry)
        if not cand or cand.random_property != 0 or cand.random_suffix != 0:
            continue
        comp_match = choose_comparator(cand, heirlooms, bracket, ssd_store, ssv_store, weights)
        if not comp_match:
            continue
        comparator, h_stats, h_dps, _ = comp_match
        item_update = compute_item_buff(
            cand,
            comparator,
            h_stats,
            h_dps,
            bracket,
            target_pct,
            rpp_store,
            gt_socket,
            damage_stores,
        )
        if item_update is not None:
            updates.append(item_update)

    return updates


def parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--target-pct",
        type=float,
        default=DEFAULT_TARGET_PCT,
        help="Target stat/DPS percentage above matching heirlooms (default: 7.0)",
    )
    parser.add_argument(
        "--weights",
        default="",
        help=(
            "Comma-separated comparator stat weights "
            "(default: stamina=0.25,crit=1.25,haste=1.25,attack_power=0.5,primary=1,hit=1,expertise=1,mastery=1,spell_power=1)"
        ),
    )
    parser.add_argument(
        "--brackets",
        default=",".join(str(x) for x in DEFAULT_BRACKETS),
        help="Comma-separated bracket endpoint levels",
    )
    parser.add_argument(
        "-o",
        "--output",
        default="-",
        help="Output SQL path (a new dated update file, never an applied one), or '-' for stdout",
    )
    parser.add_argument(
        "--dbc-dir",
        type=Path,
        default=default_dbc_dir(),
        help="Path to MoP 5.4.8 DBC directory (default: MOP_DBC_DIR or gamedata/dbc)",
    )
    parser.add_argument(
        "--db",
        default=os.environ.get("MOP_WORLD_DB", "world_gemini"),
        help="World database name (default: MOP_WORLD_DB or world_gemini)",
    )
    parser.add_argument(
        "--host",
        default=os.environ.get("MOP_DB_HOST", "127.0.0.1"),
        help="MySQL host (default: MOP_DB_HOST or 127.0.0.1)",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=int(os.environ.get("MOP_DB_PORT", "3306")),
        help="MySQL port (default: MOP_DB_PORT or 3306)",
    )
    parser.add_argument(
        "--mysql-bin",
        default=None,
        help="Path to mysql client executable",
    )
    parser.add_argument(
        "--hotfix-date",
        default=DEFAULT_HOTFIX_DATE,
        help="Fixed timestamp for hotfix_data rows (default: '2026-10-31 12:00:00')",
    )
    parser.add_argument(
        "--no-baseline",
        action="store_true",
        help="Do not overlay v1.0.4 baseline snapshot onto modified item_template rows",
    )
    return parser.parse_args(argv)


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)
    try:
        brackets = parse_brackets(args.brackets)
        weights = StatWeights.from_string(args.weights)
    except ValueError as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 2

    user = os.environ.get("MOP_DB_USER", "ultimatemop")
    password = os.environ.get("MOP_DB_PASSWORD", "ultimatemop")
    mysql_bin = find_mysql_binary(args.mysql_bin)

    try:
        updates = generate_updates(
            mysql_bin=mysql_bin,
            host=args.host,
            port=args.port,
            user=user,
            password=password,
            database=args.db,
            dbc_dir=args.dbc_dir,
            brackets=brackets,
            target_pct=args.target_pct,
            weights=weights,
            use_baseline=not args.no_baseline,
        )
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    sql_output = render_sql(
        updates,
        target_pct=args.target_pct,
        weights=weights,
        hotfix_date=args.hotfix_date,
    )

    if args.output == "-":
        sys.stdout.write(sql_output)
    else:
        out_path = Path(args.output)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_bytes(sql_output.encode("utf-8"))
        print(f"Wrote {len(updates)} item updates to {out_path}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
