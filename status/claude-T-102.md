# T-102: Creature melee damage from the client tables (and T-101 rare health)

**Agent:** claude
**Branch:** claude/T-102-creature-damage
**Task status:** done

## What changed
- Port of upstream LoA-Pandaria-5.4.8 PR 470 (still open upstream on 2026-11-06): `creature_classlevelstats`
  `damage_base`..`damage_exp3` for levels 1-85 from the client's NpcDamageByClass tables (damage per second), and
  creatures of expansion 0-3 multiply weapon damage by attack time
  (`sql/updates/world/2026_11_06_claude_01_creature_damage_tables.sql`, `StatSystem.cpp`).
- `Creature.BaseDamagePerSecond` (1) gates the formula; `CreatureDamage.h` holds the unit rule.
- AutoBalance level scaling now compares base damage per hit, since columns 0-3 are per second and column 4 per hit.
- `.npc info` prints melee min-max per hit, swing time, expansion and damage modifier.
- T-101 (PR 158, merged without a fragment): `Rares.HealthMultiplier` 2.0 for fixed rares; wild rares +95-160%.

## How it was tested
- New `creature-damage` scenario, average hit of normal warrior-class 2.0 s mobs, before -> after:
  level 10: 8.7 -> 20.9, 20: 22.7 -> 40.3, 40: 46.8 -> 82.3, 60: 78.1 -> 174.0; level 88 (MoP) unchanged at
  about 11250. Matches upstream PR 470's own table.
- Full regression suite.

## Decisions made
- Human (2026-11-06): mobs felt too weak; port upstream's fix.

## For the other agent
- Main's world database gets the SQL; back up `creature_classlevelstats` before re-running anything there.
- Upstream may merge PR 470 later; when rebasing on it, drop our SQL file or keep it (both are idempotent and equal).

## Next
- Playtest; tune per-tier with `Rate.Creature.*.Damage` if needed.
