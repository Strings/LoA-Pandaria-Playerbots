# T-089: Affixed rares

**Agent:** claude
**Branch:** claude/T-089-affixed-rares
**Task status:** done

## What changed
- **Shared affix module** (`src/server/scripts/Custom/Affixes/`): `affix_template` (hot-reloadable via `.reload`),
  `sAffixMgr` (roll, apply, reapply after evade, clear, describe), pure rules in `AffixRules.h` with unit tests.
  Host mask (rare, corruption, Brawler) so later users roll from the same table.
- **Eight affixes**, each a visible client-patched aura (spells 318001-318008): Molten (fire pulse, burst on
  death), Frozen (frost pulse), Vampiric (heals from damage), Shielded (periodic shield), Teleporting (blinks
  behind its target), Juggernaut (x2.5 health, bigger, slower, CC-immune), Summoner (adds), Sha-touched.
- **Affixed rares** (`src/server/scripts/Custom/Rares/`): open-world rares roll 0-3 affixes on spawn and respawn
  (weights 30/40/22/8). Each affix adds XP (a share of a level, 25% base, capped at 100%) and extra loot rolls.
  The killer's group gets a "Rare bonus" XP line.
- **Bots wait for a human**: a fresh rare carries Unclaimed (318000). Bots neither attack it nor get attacked by
  it until a human damages it; then it's claimed and bots join in.
- **Core hooks** (small, generic): `CreatureHooks` gives spawn/respawn/evade/remove events and a fight filter,
  read by `Creature::_IsTargetAcceptable` and playerbots' `AttackersValue` / `GrindTargetValue`.
- GM: `.rare near | spawn <entry> [affix...] | reroll [n] | set [affix...] | claim | affixes`.

## How it was tested
- Unit tests: 24/24 (new `unit_affix_rules`).
- New scenario `affixed-rares`: spawn with chosen affixes, Juggernaut health, claim on first hit, kill bonus.
- Manual, playerbots enabled in the devenv: a warrior bot stood next to an unclaimed rare for 15 s, rare stayed
  206/206; after one GM hit the bot killed it and the group got the bonus. (The regression profile runs with
  playerbots off, so this check isn't in the suite.)

## Decisions made
- Bonus XP is granted without a victim, so it shows as its own XP line and a GM kill pays too.
- The first-claim intro is a one-time notification per character per server run, kept in memory until the
  journal (Phase 1b) takes over.

## For the other agent
- SQL: `sql/updates/world/2026_11_04_claude_01_affixes.sql`, the regenerated
  `2026_11_03_claude_00_heartseed_gift_spells.sql` (now 18 spell clones), and
  `sql/updates/master/auth/2026_11_04_claude_00_rare_commands.sql` (rbac 98000).
- ID block 98000 (spells 318000-318999) is Affixes/Rares; see `docs/ID_RANGES.md`.
- Client patch needs a rebuild and install for the aura names and icons.
- New conf keys under "AFFIXED RARES (T-089)" in `worldserver.conf.dist`.

## Next
- Corruption dial and Brawler mutators roll from the same table (host bits 2 and 4).
- T-090: class pet plus Heartseed companions.

## Map additions
- `src/server/scripts/Custom/Affixes/`: shared affix module.
- `src/server/scripts/Custom/Rares/`: affixed rares and `.rare`.
- `src/server/game/Scripting/CreatureHooks.*`: creature lifecycle and fight-filter hooks.
