"""Generate SQL migration to raise trade goods (item class 7) stack sizes.

Data half of T-041 (T-062). Raises stackable on trade goods that already stack (> 1)
to a configurable size (default 1000), and emits the required client hotfix rows.
"""

import argparse
import json
import os
from pathlib import Path
import subprocess
from typing import Dict, List, Optional, Sequence, Tuple

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUTPUT = ROOT / "sql/updates/world/2026_11_01_gemini_01_trade_goods_stack_sizes.sql"
DEFAULT_JSON = Path(__file__).resolve().with_name("trade_goods.json")
HOTFIX_TYPE_ITEM = 2442913102  # 0x919E378E: Item.db2 / item hotfix table


def get_qualifying_items(
    items: Sequence[Dict[str, int]], target_size: int = 1000
) -> List[Dict[str, int]]:
    """Return items that already stack (> 1) and have stackable < target_size."""
    qualifying = []
    for item in items:
        stackable = item.get("stackable", 0)
        if 1 < stackable < target_size:
            qualifying.append(item)
    qualifying.sort(key=lambda x: x["entry"])
    return qualifying


def generate_sql(
    entries: Sequence[int],
    target_size: int = 1000,
    hotfix_date: str = "2026-11-01 12:00:00",
) -> str:
    """Generate idempotent SQL update with batch UPDATE and hotfix_data rows."""
    lines: List[str] = [
        f"-- T-062: Raise trade goods (item class 7) stackable size to {target_size}",
        f"-- Affects {len(entries)} items with existing stackable > 1",
        "-- Matching item hotfix rows for MoP 5.4.8 client sync",
        "",
        "START TRANSACTION;",
        "",
        f"UPDATE item_template SET stackable = {target_size} WHERE class = 7 AND stackable > 1 AND stackable < {target_size};",
        "",
    ]

    if entries:
        entry_list_str = ",".join(str(e) for e in sorted(entries))
        lines.append(
            f"DELETE FROM hotfix_data WHERE type = {HOTFIX_TYPE_ITEM} AND hotfixDate = '{hotfix_date}' AND entry IN ({entry_list_str});"
        )

        # Batch INSERT hotfix_data rows
        batch_size = 200
        values_batches = []
        for i in range(0, len(entries), batch_size):
            batch = entries[i : i + batch_size]
            val_strs = [f"({e},{HOTFIX_TYPE_ITEM},'{hotfix_date}')" for e in batch]
            values_batches.append(f"INSERT INTO hotfix_data (entry, type, hotfixDate) VALUES {','.join(val_strs)};")

        lines.extend(values_batches)
        lines.append("")

    lines.append("COMMIT;")
    lines.append("")

    return "\r\n".join(lines)


def fetch_from_mysql(
    host: str = "127.0.0.1",
    user: str = "ultimatemop",
    password: str = "ultimatemop",
    database: str = "world_gemini",
) -> Optional[List[Dict[str, int]]]:
    """Query item_template from MySQL directly if client is available."""
    try:
        cmd = [
            "mysql",
            "-h",
            host,
            "-u",
            user,
            f"-p{password}",
            database,
            "-BNe",
            "SELECT entry, name, stackable FROM item_template WHERE class = 7 ORDER BY entry;",
        ]
        res = subprocess.run(cmd, capture_output=True, text=True, check=True)
        items = []
        for line in res.stdout.strip().splitlines():
            if not line:
                continue
            parts = line.split("\t")
            items.append({"entry": int(parts[0]), "name": parts[1], "stackable": int(parts[2])})
        return items
    except Exception:
        return None


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--size",
        type=int,
        default=1000,
        help="Target stack size for stacking trade goods (default: 1000)",
    )
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help=f"Output SQL file path (default: {DEFAULT_OUTPUT})",
    )
    parser.add_argument(
        "--database",
        default=os.environ.get("MOP_WORLD_DB", "world_gemini"),
        help="MySQL world database name (default: world_gemini)",
    )
    parser.add_argument(
        "--db-user",
        default=os.environ.get("MOP_DB_USER", "ultimatemop"),
        help="MySQL user (default: ultimatemop)",
    )
    parser.add_argument(
        "--db-pass",
        default=os.environ.get("MOP_DB_PASSWORD", "ultimatemop"),
        help="MySQL password (default: ultimatemop)",
    )
    parser.add_argument(
        "--db-host",
        default=os.environ.get("MOP_DB_HOST", "127.0.0.1"),
        help="MySQL host (default: 127.0.0.1)",
    )
    parser.add_argument(
        "--items-json",
        type=Path,
        default=DEFAULT_JSON,
        help="Fallback JSON file with trade goods items",
    )
    parser.add_argument(
        "--hotfix-date",
        default="2026-11-01 12:00:00",
        help="Hotfix date stamp (default: 2026-11-01 12:00:00)",
    )
    args = parser.parse_args()

    items = fetch_from_mysql(args.db_host, args.db_user, args.db_pass, args.database)
    if items is None and args.items_json.is_file():
        items = json.loads(args.items_json.read_text(encoding="utf-8"))

    if not items:
        print("Error: Could not retrieve trade goods from MySQL or items JSON file.")
        return 1

    qualifying = get_qualifying_items(items, target_size=args.size)
    entries = [item["entry"] for item in qualifying]
    sql_text = generate_sql(entries, target_size=args.size, hotfix_date=args.hotfix_date)

    args.output.parent.mkdir(parents=True, exist_ok=True)
    with open(args.output, "wb") as f:
        f.write(sql_text.encode("utf-8"))

    print(f"Generated {args.output} for {len(entries)} items (target stack size: {args.size})")
    return 0


if __name__ == "__main__":
    import sys
    sys.exit(main())
