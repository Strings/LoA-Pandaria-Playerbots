# Stack Sizes Generator (T-062 / T-041)

Generator that creates world SQL updates to raise stackable limits on trade goods
(item class 7) that already stack (> 1) to a configurable size (default: 1000),
accompanied by matching item hotfix rows (`hotfix_data`, type 2442913102).

## Usage

```bash
# Generate SQL update with default 1000 stack size:
python tools/stack-sizes/stack_sizes.py

# Custom stack size:
python tools/stack-sizes/stack_sizes.py --size 500 --output sql/updates/world/custom_stacks.sql
```

## Flags

- `--size`: Target stack size for stacking trade goods (default: 1000).
- `--output`: Output file path (default: `sql/updates/world/2026_11_01_gemini_01_trade_goods_stack_sizes.sql`).
- `--database`: Target world MySQL database name (default: `world_gemini`).
- `--db-user`, `--db-pass`, `--db-host`: MySQL connection settings.
- `--items-json`: Path to bundled JSON trade goods dataset fallback.

## Testing

```bash
python -m unittest discover -s tools/stack-sizes
```
