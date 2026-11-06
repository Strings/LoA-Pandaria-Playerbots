# T-104: Battleground prep 1 minute; Hurl Axe works; wild rare auras show

**Agent:** claude
**Branch:** claude/T-104-bg-prep-hurl-axe
**Task status:** review

## What changed
- New `Battleground.PrepTimeSeconds` (60) in `worldserver.conf.dist`. `Battleground::ApplyPrepTimeConfig()` runs before
  `SetupBattleground()` and remaps the warning slots (2 min / 1 min / 30 s / start) onto the configured time.
  `GetPrepCountdownSeconds()` feeds the client countdown (ProcessJoin and `.bg` in `cs_misc.cpp`). Temple of Kotmogu's
  door timer follows the shortened start. A value of 0, or one at or above a battleground's own time, keeps the stock
  time. Arenas are unchanged.
- Hurl Axe (multiclass 320000):
  - 1 s cast, castable while moving.
  - 6 s cooldown, no cost, generates 20 Rage.
  - Deals 150% weapon damage plus 80% of Mortal Strike's (12294) level-scaled bonus damage.
- `multiclass_ability` gains four columns: `bonus_spell`, `bonus_effect`, `bonus_pct`, `power_gain`
  (`sql/updates/world/2026_11_06_claude_20_multiclass_weapon_abilities.sql`).
- Multiclass weapon-percent clones now use main-hand weapon damage. The thrown clone used the ranged attack type, and MoP
  warriors have no ranged weapon, so it dealt 0.
- Wild rare markers: the 10 aura clones of 33208 now clear `SPELL_ATTR1_DONT_DISPLAY_IN_AURA_BAR`, which had hidden them
  client-side (`tools/clientpatch/edits.json` and the regenerated `2026_11_03_claude_00_heartseed_gift_spells.sql`).
  Seeing the auras needs the new client patch MPQ.

## How it was tested
- Warsong Gulch on the Claude test server, level 57 with bots on: popped at once. The chat showed "begins in 1 minute"
  and then "30 seconds".
- Hurl Axe: a level 40 Barbarian against a 100000 HP boar. The cast took 1 s. Damage was about 199 weapon plus about 217
  Mortal Strike bonus. The Rage gain was not measured directly: `.modify rage` fails in `world_claude`.
- Wild rare auras: data change only. The client display is untested until the patch is installed.
- Follow-up (PR 2): Hurl Axe would not cast on the move. Its 1 s cast kept Throw Axe's movement interrupt
  (`InterruptFlags` 15). `edits.json` `interrupts.clear_cast` and a SpellMgr correction clear it. New headless
  verb `move-start` and scenario `hurl-axe-moving` (passes).
- Regression suite: see the PR. `multiclass` and `telemetry` already fail in this env.

## Decisions made
- The prep time is one global key for all battlegrounds, not per battleground.

## For the other agent
- Apply both world SQL files. Rebuild and install the client patch (`tools/clientpatch/build.py build` / `install`).

## Next
- Playtest the Hurl Axe numbers at low level against Mortal Strike.

## Map additions
- none
