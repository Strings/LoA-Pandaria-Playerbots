# T-048: GM test suite (checkpoints, kits, ability tester, addon)

**Agent:** claude
**Branch:** claude/T-048-gm-suite
**Task status:** review

## What changed
- **Checkpoints** (`src/server/scripts/Custom/GMTools/Checkpoints/`): named templates of ordered, typed steps in
  `gmtools_checkpoint` and `gmtools_checkpoint_step`. `.checkpoint list | explain | apply | reload`.
  Step types are registered by systems (`CheckpointMgr::RegisterStep`), so later systems add their fields
  without a schema change. The core steps are `level`, `class_spells`, `gear`, `kit`, `item`, `spell`, `quest`,
  `reputation`, `tele`, and `command`, the escape hatch. The Heartseed adds `heartseed`, `_rank`, `_growth`,
  `_major`, `_flag` and `_essence`.
- **Nine checkpoints:** `hs-10` to `hs-85` at each major tier, `legendary-1` and `legendary-2`, and `max-90`.
  Each sets the level, class spells, bags, gear (playerbots gear factory), Heartseed rank at the cap, explicit
  story flags and a cleared choice for its own tier, then teleports to the faction capital or shrine.
- **Kits:** `gmtools_kit` and `gmtools_kit_item` (per class, equip or bags), `.kit list | explain | apply`. Only a
  `bags` kit ships; twink kits come with T-045's gear list.
- **Ability tester:** `.gmsuite refresh` resets cooldowns, fills health and every power, and revives. The core
  `.cheat` toggles cover the rest.
- **GMSuite addon** (`addons/GMSuite/`, `/gms`):
  - Checkpoint and kit buttons. A checkpoint asks to confirm, because a level-down can't be undone.
  - Ability tester buttons, a set-level box and a learn-spell box.
  - The lists come from the server through `.gmsuite sync` (addon messages, prefix `GMSuite`).
- **ID block:** GM tools take the 91000 block in `docs/ID_RANGES.md`. RBAC 91000 to 91002 are linked to role 197.
- **Permission 1345:** the auth update adds RBAC permission 1345 (`.pmon`, `.botscenario`, `.probe`) and links it
  to role 196 (Administrator Commands). The playerbots module never shipped that row, so only the console could
  run those commands.

## How it was tested
Claude test server, headless client, `Seedtest` (GM). The world SQL was applied twice to `world_claude` and the
auth SQL twice to `auth_claude`.
- `.checkpoint list` (SOAP and chat) and `.checkpoint explain hs-25`.
- `.checkpoint apply hs-10` from level 30: level 10, rank 10 at cap 10, no flags, level-10 major cleared, in
  Stormwind. 0 of 12 steps failed.
- `.checkpoint apply max-90`: level 90, rank 105 at cap 105, all three flags, Shrine of Seven Stars. 0 failed.
- `.checkpoint apply hs-25` after the permission fix: green gear equipped (item level 27 to 35), rank 30 at cap
  30, first_dungeon set.
- `.kit apply bags` skips full bag slots. `.gmsuite refresh` revived and refreshed. `.gmsuite sync` delivered
  nine `C` lines, one `K` line and `E` as addon whispers.
- Addon: the Lua parses (luaparser). It has not been loaded in the client yet. That needs the game client
  (computer use), so it is left for the human playtest.
- Built with no new warnings in touched files.

## Decisions made
- **Gear through `.botscenario equip`** instead of linking the scripts library to the playerbots module's
  headers: same gear factory, no build coupling. It upgrades only, so a high checkpoint followed by a low one
  keeps the better gear.
- **Ranks follow the cap** (level + 5 per awakening, with awakenings assumed at 15, 85 and 90). Legendary levels 60 and 80
  are placeholders, as the task says.
- **The addon confirms before applying a checkpoint.** That is two clicks rather than the one in the brief: a
  level-down or a cleared major can't be undone.
- Checkpoints leave lower tiers' majors alone, so a choice made earlier survives moving up. Each clears only its
  own tier.

## For the other agent
- Apply both SQL files. `.botscenario` now works from chat for administrators (permission 1345).
- New systems: register checkpoint steps for your state (see `checkpoint_heartseed.cpp`) and add them to the
  checkpoint rows.
- T-004 telemetry should pause around `CheckpointMgr::Apply`.

## Next
- Twink kits once T-045 lands; the T-039 GM tools share the `GMTools/` folder.

## Map additions
- `src/server/scripts/Custom/GMTools/Checkpoints/`: checkpoint templates and kits (GM test suite).
- `addons/`: client addons we ship (GMSuite). Copy them into a client copy's `Interface\AddOns\`.
