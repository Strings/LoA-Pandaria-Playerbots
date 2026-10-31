# T-020: Heartseed state table and weapon glow

**Agent:** claude
**Branch:** claude/T-020-heartseed-state
**Task status:** review

## What changed
- New module `src/server/scripts/Custom/Heartseed/` (README inside): `HeartseedMgr` caches each
  character's Heartseed state, loads it at login, saves it with the character, and works out the
  stage and glow. `HeartseedRules.h` holds the pure rules (stage by level, dominant path) with no
  server headers, ready for T-008 unit tests.
- Characters tables: `heartseed_state` (rank, Growth, grant time; a row means "has a Heartseed"),
  `heartseed_loadout` (major per tier per specialization), `heartseed_essence` (collected, active
  slot), `heartseed_story_flag` (beat and time).
- World tables: `heartseed_stage` (the six stages by level from DESIGN.md) and `heartseed_glow` (glow
  enchant per stage and dominant path, path 0 = none).
- Weapon glow: new `PlayerScript::OnVisibleItemEnchant` hook, called from `Player::SetVisibleItemSlot`
  and when a temporary enchant ends. The Heartseed supplies the main hand's glow in the
  temporary-enchant half of the visible item field.
- GM commands (permission 90000, Gamemaster role): `.heartseed info | grant | revoke`.
- Config: `Heartseed.Enabled` (default 1) in `worldserver.conf.dist`.
- `docs/ID_RANGES.md`: RBAC permissions use the low end of a system's gossip block.

## How it was tested
- SQL updates run twice against `characters_claude`, `world_claude`, `auth_claude`: no errors.
- Server start on the Claude test server (bots off): `Loaded 6 Heartseed stages and 6 glows`, no DB errors.
- Headless client, level-1 human warrior `Seedtest` on account `CLAUDE`, steps through `say .heartseed ...`:
  - no Heartseed: main-hand visible enchants permanent 0, temporary/glow 0;
  - `.heartseed grant` at level 10: stage 1 (Sprout), field shows glow 4446;
  - relog: state reloads, field shows 4446 again;
  - `.levelup 20` (level 30): stage 2 (Sapling), field shows 4441;
  - second `grant` refused ("already has a Heartseed");
  - `.heartseed revoke`: field back to 0, `heartseed_state` has no rows.
- `.heartseed info` prints the visible item field itself, so this check needs no debug commands.
- Not checked: a real temporary enchant taking precedence over the glow (needs a rogue poison or shaman
  imbue), and how the glows look in the client.

## Decisions made
- **Glow without a real enchant on the item.** DESIGN.md (Implementation) says a hidden enchant goes in
  a free enchant slot. Any enchant in an item slot has its effects applied by the core, and the client
  data has only five enchants with a visual and no effects, too few for stage and path glows. So the
  glow is written only to the visible item field: same look for everyone who sees the character, no
  effects, and nothing travels with a traded weapon. Trait links don't need the slot: traits are
  learned passive auras (DESIGN.md). Easy to change if the slot is wanted later.
- A real temporary enchant (poison, oil, imbue) shows instead of the glow while it lasts.
- Glow enchant IDs are placeholders with visible effects (Crusader, River's Song, Windsong, Jade
  Spirit, Dancing Steel); the final looks need someone to view them in the client.
- Story flags are rows (flag, time) rather than a bitmask in the state row, so beats are data and keep
  their time for the chronicle.
- Bots skip Heartseed loading (no queries per bot login).
- Stage follows character level, as in the DESIGN.md stage table.

## For the other agent
- New hook in a shared header (`ScriptMgr.h`), so expect a near-full rebuild after merging.
- SQL to apply to your databases: `sql/updates/characters/2026_10_31_claude_00_heartseed_state.sql`,
  `sql/updates/world/2026_10_31_claude_01_heartseed_stage.sql`,
  `sql/updates/master/auth/2026_10_31_claude_02_heartseed_command.sql`.

## Next
- T-021 adds `heartseed_trait`; `HeartseedMgr::GetDominantPath` then counts majors per path.
- Not run against a copy of a real save: no save exists yet (T-037).
- No unit tests yet: the framework is T-008. `HeartseedRules.h` is ready for them.
- The human should look at the glows in the client and pick the final enchant IDs.

## Map additions
- `src/server/scripts/Custom/Heartseed/`: the Heartseed module.
