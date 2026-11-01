# Spec loadouts (T-017)

Any number of server-side loadouts per character: one per specialization, created automatically (four for druids),
plus named extras (`Loadouts.MaxNamed`, default 10). A loadout holds the spec, talents, glyphs, action bars and the
parts other systems register (the Heartseed's majors today; gear pages in T-018).

## How it works

The 5.4.8 client knows exactly two talent groups. Each group holds one loadout (`character_loadout.talent_group`).
Switching to a loadout:

1. captures the active group into its loadout, so nothing changed since the last save is lost;
2. if the inactive group already holds the target, it just runs the normal spec switch;
3. otherwise it captures the inactive group into its loadout, then writes the target's spec, talents and bars into
   the inactive group, applies the registered parts, runs `Player::ActivateSpec`, sets the glyphs and saves.

Changing talents or the spec through the normal client UI works too: the group's loadout is captured on every save,
and a group that changes spec is re-attached to that spec's own loadout. A loadout that never held anything
("empty") starts from the current action bars.

Out of combat, alive and outside arenas only. Bots have no loadouts.

## Interface

- `LoadoutMgr` (`sLoadoutMgr`): `Switch`, `Create` (a copy of the active loadout), `Rename`, `Delete`, `GetLoadouts`,
  `GetActiveId`, `Resolve` (id or name), `CaptureActive`.
- **Parts:** `sLoadoutMgr->RegisterPart({ name, Capture(player, specId) -> string, Apply(player, specId, data) })`.
  Capture runs whenever the loadout is captured; Apply runs before the spec switch, so the system's own
  `OnSpecializationChanged` handling sees the new values. `loadout_heartseed.cpp` is the example.
- `LoadoutRules.h`: name rules, id allocation, default-loadout planning and id/name lookup (unit tested in
  `tests/loadouts/`).

## Commands

| Command | Who | Does |
|---|---|---|
| `.loadout` / `.loadout list` | players | List loadouts |
| `.loadout switch <id or name>` | players | Switch |
| `.loadout new <name>` | players | Save the current setup as a new named loadout |
| `.loadout rename <id or name> <new name>` | players | Rename |
| `.loadout delete <id or name>` | players | Delete a named loadout no talent group holds |
| `.loadout save` | players | Save now |
| `.loadout sync` | players | Send the list to the addon |
| `.loadout inspect [player]` | GMs | Show another player's loadouts and talent groups |

## Config

`Loadouts.Enable` (1), `Loadouts.MaxNamed` (10).

## Tables (characters)

`character_loadout`, `character_loadout_talent`, `character_loadout_glyph`, `character_loadout_action`,
`character_loadout_part`. Created by `sql/updates/characters/2026_11_01_claude_00_loadouts.sql`; rows go with
the character on delete.
