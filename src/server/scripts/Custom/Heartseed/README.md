# Heartseed

The living jewel that rides on a character's main-hand weapon and grows with it. Design:
`docs/DESIGN.md`, "The Heartseed". IDs come from the Heartseed block in `docs/ID_RANGES.md`.

## Files

| File | What |
|---|---|
| `HeartseedRules.h` | Pure rules with no server dependencies (stage by level, dominant path). Unit-test these. |
| `HeartseedMgr.h/.cpp` | State cache per character, loading and saving, stage and glow lookup. |
| `heartseed_scripts.cpp` | World, player and command scripts. |

## Data

Characters database (`sql/updates/characters/2026_10_31_claude_00_heartseed_state.sql`):

- `heartseed_state`: one row per character that has a Heartseed (rank, Growth, grant time). Having a
  row is what "has a Heartseed" means to code; the granting quest (T-025) creates it.
- `heartseed_loadout`: the major chosen at each of the six major tiers, per specialization.
- `heartseed_essence`: essences collected and which slot (1 to 3) each is active in.
- `heartseed_story_flag`: story beats reached and when (`Heartseed::StoryFlag`).

World database (`sql/updates/world/2026_10_31_claude_01_heartseed_stage.sql`):

- `heartseed_stage`: stage number, starting character level, name.
- `heartseed_glow`: glow enchant per stage and dominant path (path 0 = none).

## Weapon glow

The glow is display only. `Player::SetVisibleItemSlot` asks `PlayerScript::OnVisibleItemEnchant` for a
visual whenever the temporary-enchant half of the visible item field is empty, and the Heartseed answers
for the main hand with the `heartseed_glow` enchant for the character's stage. Nothing is written to the
item, so the enchant's effects never apply and the glow never travels with a traded weapon. A real
temporary enchant (poison, oil, imbue) shows instead while it lasts.

The glow enchant IDs shipped are placeholders that have a visual; pick the final looks in the client.

## Config

`Heartseed.Enabled` (worldserver.conf, default 1): off keeps state but shows no glow.

## GM commands

Permission 90000 (`Command: heartseed`, Gamemaster role). Target is the selected player or yourself.

- `.heartseed info`: rank, Growth, stage, loadout, essences, glow, story flags and the main hand's
  visible enchant field as the client receives it.
- `.heartseed grant`: give a Heartseed (rank 1, no Growth).
- `.heartseed revoke`: remove it with all its rows.
- `.heartseed rank <1-255>`: set the rank. Ignores the level cap, so states can be tested without leveling.
- `.heartseed growth <n> | +<n> | -<n>`: set Growth, or add to or take from it (never below 0).
- `.heartseed flag list | set <name|id> | clear <name|id>`: story flags by name (`first_dungeon`,
  `reached_pandaria`, `first_world_boss`) or number. Setting keeps the first time a flag was set.

Every change is saved at once, not with the next character save.
