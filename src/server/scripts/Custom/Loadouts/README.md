# Spec loadouts (T-017) and gear pages (T-018)

Any number of server-side loadouts per character: one per specialization, created automatically (four for druids),
plus named extras (`Loadouts.MaxNamed`, default 10). A loadout holds the spec, talents, glyphs, action bars and the
parts other systems register (the Heartseed's majors today), and a gear page.

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

## Gear pages

Each loadout has a page of equipment slots held in detached storage (`../DetachedStorage/`), so off-spec gear lives
on its page instead of in the bags. A page holds only the pieces that differ: switching to a loadout stores the worn
pieces in the slots its page fills (plus the off hand when a two-hander comes in, or a two-hander when an off hand
comes in) onto the outgoing loadout's page, then equips the incoming pieces through the normal equip path, main hand
first. Every other slot stays as it is, so a fresh loadout (empty page) keeps the current gear. A piece that can't be
worn (level, unique-equipped) goes to the bags; if the bags are full it stays on the page as a spare (slot 100+).

The swap runs after every change of the worn loadout: `.loadout switch` and the client's own spec switch alike. The
worn loadout's page is what you have on, so pieces go on other loadouts' pages only. A loadout whose page holds
pieces can't be deleted. Rules: `GearPageRules.h` (unit tested in `tests/loadouts/`).

## Interface

- `LoadoutMgr` (`sLoadoutMgr`): `Switch`, `Create` (a copy of the active loadout), `Rename`, `Delete`, `GetLoadouts`,
  `GetActiveId`, `Resolve` (id or name), `CaptureActive`.
- **Parts:** `sLoadoutMgr->RegisterPart({ name, Capture(player, specId) -> string, Apply(player, specId, data) })`.
  Capture runs whenever the loadout is captured; Apply runs before the spec switch, so the system's own
  `OnSpecializationChanged` handling sees the new values. `loadout_heartseed.cpp` is the example.
- **Listeners:** `sLoadoutMgr->RegisterListener({ OnSwitched(player, fromId, toId), CanDelete(player, id) })` for
  systems that follow the worn loadout themselves; gear pages (`loadout_gearpages.cpp`) are the one today.
- `GearPages::Swap`, `Put`, `Take` (`GearPages.h`).
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
| `.loadout page [id or name]` | players | Show one gear page, or every page with pieces on it |
| `.loadout page put <loadout> <bag> <slot> [equipment slot]` | players | Put a piece from the bags on a page (bag 0-4, slot from 1, as the client counts) |
| `.loadout page take <loadout> <page slot>` | players | Take a piece back to the bags |
| `.loadout inspect [player]` | GMs | Show another player's loadouts, gear pages and talent groups |

## Config

`Loadouts.Enable` (1), `Loadouts.MaxNamed` (10), `Loadouts.GearPages.Enable` (1).

## Tables (characters)

`character_loadout`, `character_loadout_talent`, `character_loadout_glyph`, `character_loadout_action`,
`character_loadout_part`. Created by `sql/updates/characters/2026_11_01_claude_00_loadouts.sql`; rows go with
the character on delete.
