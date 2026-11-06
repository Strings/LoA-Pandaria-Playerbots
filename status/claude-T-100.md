# T-100: Wild rares

**Agent:** claude
**Branch:** claude/T-100-wild-rares
**Task status:** done

## What changed
- Wild rares (`Custom/WildRares/`, design in DESIGN.md "Rares and discovery", decided 2026-11-06): eligible ordinary
  hostile open-world mobs roll 2% per spawn or respawn (zone cap 6, spawn-point cooldown 900 s) to become a wild rare
  with one or two affixes at 55% intensity, +50 to 100% health, the *Wild Rare* marker aura (318100), a spawn emote
  and a minimap star. A human player's death near one (in combat with it, killed by it, or within 40 yd) wears it off.
  The kill pays 10% of a level plus 5% per affix (cap 20%), one extra loot roll and a 3% chance at a Heartseed essence.
- Affix module: `Apply(creature, ids, intensity, host)` with `Intensity(guid)` and `Host(guid)`; amounts scale with
  intensity, amount-less affixes fire less often, add counts never drop below one. New host bit `HOST_WILD_RARE` (8).
  `Clear` now removes the auras before forgetting, so Juggernaut takes back exactly what it added.
- Fixed rares skip affixes another host applied (no double payout, no Unclaimed on a wild rare after an evade).
- Vignettes: `CreatureHooks::RegisterVignetteResolver` gives creatures a minimap star their template lacks;
  `VignetteMgr.cpp` asks it, removes stars by source and never shows two for one creature. Pre-MoP rares and rare
  elites had none (0 of 790 in world data), so they now get the generic kill star (`Rares.Vignette.Id`, 4).
- `.wildrare spawn [entry] [affix ...] | explain | reset | stats | chance [pct|reset]` (permission 98001).

## How it was tested
- Unit tests: `unit_wild_rare_rules` (eligibility reasons, roll, cap, cooldown, health, XP) and new intensity checks
  in `unit_affix_rules`.
- Regression scenario `wild-rares.json`: forces a wild rare, lets it kill a 1 hp player and checks the death reset,
  re-wilds it, kills it with `.die` and checks the bonus, and checks stats and the chance override. Passes; full
  suite 36 of 37 (telemetry times out: its scenario walks to Elwynn without teleporting first, so it fails whenever
  an earlier scenario leaves the character elsewhere; unrelated to this change).
- Not verified in a real client: the minimap stars (the headless client doesn't track vignettes), the marker's
  client-patched name and icon.

## Decisions made
- "Has a fixed-rare row" read as rare or rare-elite rank: there is no fixed-rare table; those creatures are the fixed
  tier, and elites and bosses are excluded too.
- Neutral (yellow) mobs count as hostile by default (`WildRares.IncludeNeutral`): much of the leveling world is neutral.
- After the death reset the creature is fully ordinary (no marker, star or bonus), so dying never leaves an easier
  rare that still pays. Only human deaths count by default (`WildRares.DeathReset.BotsCount = 0`).
- The essence is a random one from `heartseed_essence_template`; wild rares have no origin to theme it by.
- The minimap star reuses Vignette.db2 row 4 ("Vignette Test Kill"). Its name may show on hover until the client
  patch can edit DB2 rows (noted in ART-NEEDED.md).
- Bots don't wait for a human on wild rares (no Unclaimed hold): they're the lesser tier.

## For the other agent
- SQL: `sql/updates/world/2026_11_06_claude_10_wild_rares.sql` (denylist table, affix host bit, marker script),
  `sql/updates/master/auth/2026_11_06_claude_10_wildrare_commands.sql`, and the regenerated clone file
  `2026_11_03_claude_00_heartseed_gift_spells.sql` (adds 318100).
- Client patch: `edits.json` gains spell 318100; rebuild and install the archive on deploy.
- New config block `WildRares.*` and `Rares.Vignette.Id` in `worldserver.conf.dist`.
- `CreatureHooks.h` gained the vignette resolver (small header; a few files recompile).

## Next
- Tune `WildRares.ChancePct` down after playtesting; add problem entries to `rare_wild_denylist`.
- A Vignette.db2 row of our own named "Wild Rare" once the client patch edits DB2 tables.

## Map additions
- `src/server/scripts/Custom/WildRares/`: wild rares (lesser random rares), README.
- `tests/wild_rares/`: wild rare rule tests.

## Follow-up fixes after the first playtest (T-100b)
- No buffs on wild or fixed rares: the core marks every spell id from 203751 up as server-only and never sends its
  aura. `build.py sql` now lists each client-patched clone in `spell_client_known` (world), and
  `SpellMgr::LoadSpellInfoCustomAttributes` exempts them. This also makes the Heartseed rider auras and Unclaimed
  visible, as their clones intended.
- Star hover said "Vignette Test Kill": the client patch renames Vignette.db2 row 4 to "Rare" (`dbc` edits now handle
  WDB2 tables and string cells; `tools/dbc` gained a Vignette schema).
- Stars stayed on corpses: `Creature::setDeathState` takes a creature's star away from players who see it when it dies
  and gives it back when it respawns in place; `VignetteMgr` never shows a star on a dead creature. Applies to every
  creature with a star (stock MoP rares too).
- Tested: clientpatch unit tests (string edit on WDB2), the wild-rares scenario now checks the marker and an affix
  aura get a client slot (`.list auras`). Real-client look still needed for the stars.
