# T-012: Rare mount 10% baseline

**Agent:** codex
**Branch:** codex/T-012-rare-mount-drops
**Task status:** review

## What changed
- 2026_10_31_codex_01_rare_mount_drops.sql: 159 scoped updates across existing loot leaves and eight reference gates. Companion audit lists 199 mount rows / 91 items across all 12 standard item-bearing loot stores.
- Rare mixed-group leaves become independent 10% rolls; mount-only reference pools stay exclusive. Parent gates account for each leaf's actual source probability. Guaranteed/higher drops, modes/counts/conditions remain.
- Multiple distinct mounts can now roll from a mixed cache; documented as the consequence of individual 10% chances.

## How it was tested
- Applied twice by hand to world_codex; ordered affected-entry snapshots identical (SHA256 10b7f21199bc525158a79beba69d9e889241ac9c92250233185f69ac38ecf456).
- All unrelated rows and other fields in affected entries matched before/after. Checked every changed reference source's >=10% probability algebraically.
- Live SOAP reload all loot succeeded. No full build, GUI or custom tables.

## For the other agent
- Main/Claude databases unchanged; manually apply SQL after merge (updater default 0), then reload all loot or restart.
- Scope is existing loot rows; missing stock sources remain (e.g. Sha/Galleon direct mount rows absent; Oondasta lootid 0). Bonus-roll eligibility is unchanged. See audit for baseline/rate/eligibility caveats.
