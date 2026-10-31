# Twink Buff Generator (`tools/twink-tuning/`)

Regenerates the twink gear stat-buff SQL migration (`sql/updates/world/2026_10_31_gemini_00_twink_gear.sql`, replacing `2026_10_31_codex_00_twink_gear.sql`) from live read-only `item_template` data and MoP 5.4.8 DBC scaling curves (`ScalingStatDistribution.dbc`, `ScalingStatValues.dbc`, `RandPropPoints.dbc`, `gtItemSocketCostPerLevel.dbc`, `ItemDamage*.dbc`), so twink tuning can be adjusted after playtests without repeating the manual T-045 research.

See `docs/twinks.md` for the candidate shortlist rules, heirloom matching logic, and per-item audit.

## Method Summary

1. **Shortlist Selection:** Queries the top 2 non-heirloom items per `(bracket, Class, SubClass, InventoryType)` across the configured bracket endpoints and evaluates each item at the **first endpoint** where it appears (`RandomProperty` / `RandomSuffix` items are excluded).
2. **Comparator Matching:** Matches each candidate against `Quality = 7` heirlooms sharing `Class`, `SubClass`, normalized slot (robes `20` and chests `5` share the chest slot), identical primary-stat set (`Agility=3`, `Strength=4`, `Intellect=5`), and caster-weapon flag (`FlagsExtra & 0x200`). Heirlooms are evaluated at `min(endpoint, ssd.MaxLevel)` and ranked by weighted stat score.
3. **Stat & Weapon DPS Floors:**
   - Primary and offensive heirloom stats (`3, 4, 5, 31, 32, 36, 37, 38, 39, 45, 49`) receive a `ceil((1 + target_pct / 100) * heirloom_stat)` floor. Existing higher values are kept; missing offensive stats use empty stat slots; Stamina (`7`) is never increased.
   - Weapons receive a `(1 + target_pct / 100) * heirloom_dps` floor by advancing `ItemLevel` to the smallest sufficient level in `ItemDamage*.dbc`.
   - All modified stat allocations (`stat_unk1_N`)—and existing non-zero stats on weapons whose `ItemLevel` increased—are rebased to the final `ItemLevel` (including `gtItemSocketCostPerLevel.dbc` socket penalties).
4. **Idempotent Baseline Overlay:** `v104_baseline.json` stores the stock v1.0.4 `ItemLevel` and `stat_*` values for the 314 modified items so re-running the generator against a database that already has the twink migration applied remains 100% read-only and deterministic.

## Arguments & Environment Variables

| Flag / Variable | Default | Description |
|---|---|---|
| `--target-pct` | `7.0` | Target stat and weapon DPS percentage above matching heirlooms (`7.0` = `1.07x` floor). |
| `--weights` | `stamina=0.25,crit=1.25,haste=1.25,attack_power=0.5,primary=1,hit=1,expertise=1,mastery=1,spell_power=1` | Comma-separated `stat=weight` overrides for comparator ranking. |
| `--brackets` | `19,29,39,49,59,60,69,70,78,79,80,84,85,89` | Comma-separated bracket endpoint levels. |
| `-o`, `--output` | `sql/updates/world/2026_10_31_gemini_00_twink_gear.sql` | Output SQL file path, or `-` for stdout. |
| `--dbc-dir` / `MOP_DBC_DIR` | `I:\UltimateMoP\gamedata\dbc` | Directory containing MoP 5.4.8 `.dbc` files. |
| `--db` / `MOP_WORLD_DB` | `world_gemini` | Read-only target world database name. |
| `--host` / `MOP_DB_HOST` | `127.0.0.1` | MySQL host. |
| `--port` / `MOP_DB_PORT` | `3306` | MySQL port. |
| `MOP_DB_USER` / `MOP_DB_PASSWORD` | `ultimatemop` | Read-only database credentials. |

## Example Runs

Reproduce the default 7% migration (`314` item updates) and diff against the T-045 reference file:

```powershell
$env:MOP_DB_USER = "ultimatemop"
$env:MOP_DB_PASSWORD = "ultimatemop"
python tools/twink-tuning/twink_tuning.py
git diff --no-index sql/updates/world/2026_10_31_codex_00_twink_gear.sql sql/updates/world/2026_10_31_gemini_00_twink_gear.sql
```

Preview a 5% tuning pass for classic x9 brackets to stdout:

```powershell
python tools/twink-tuning/twink_tuning.py --target-pct 5 --brackets 19,29,39,49,59 --weights "stamina=0.25,crit=1.3,haste=1.3,ap=0.5" -o -
```

## Running Unit Tests

```powershell
python -m unittest discover -s tools/twink-tuning -v
```
