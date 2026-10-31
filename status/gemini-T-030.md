# T-030: Shared DBC reader library and MCP content search

**Agent:** gemini
**Branch:** gemini/T-030-dbc-content-search
**Task status:** review

## What changed
- Added `tools/dbc/` (`reader.py`, `schemas.py`, `__init__.py`, `test_dbc.py`, `README.md`):
  - Dependency-free Python 3.9+ reader for MoP 5.4.8 (build 18414) `WDBC` (`.dbc`, 20-byte header)
    and `WDB2` (`.db2`, 32/48-byte header + extended `(max_index - min_index + 1) * 6` lookup
    block) files.
  - Supports 28 named `TableSchema` definitions (`Spell`, `SpellMisc`, `SpellEffect`, `SpellIcon`,
    `SpellCooldowns`, `SpellLevels`, `SpellDuration`, `SpellRange`, `SpellCastTimes`,
    `SpellCategories`, `SpellItemEnchantment`, `ItemVisuals`, `ItemVisualEffects`, `SpellVisual`,
    `SpellVisualKit`, `SpellVisualEffectName`, `SpellVisualKitModelAttach`, `SpellVisualMissile`,
    `Item`, `ItemSparse`, `ItemDisplayInfo`, `Creature`, `CreatureDisplayInfo`,
    `CreatureModelData`, `CreatureFamily`, `CreatureType`, `GameObjectDisplayInfo`, `GameObjects`),
    TrinityCore format strings (`DBCfmt.h` / `DB2fmt.h` syntax), and raw column auto-inference.
  - Locale-aware `DbcDirectory` resolves non-empty `.dbc`/`.db2` files across `<dbc_dir>/<locale>/`
    and `<dbc_dir>/` (automatically skipping 0-byte root placeholders such as `SpellVisual.dbc` in
    favor of `enUS/SpellVisual.db2`).
- Added `tools/content-search/` (`search_engine.py`, `server.py`, `test_server.py`, `README.md`):
  - Core `ContentSearchEngine` built on `tools/dbc/` plus an MCP `2024-11-05` stdio JSON-RPC 2.0
    server and `--cli` adapter exposing:
    - `search_spells` (joins `Spell.dbc`, `SpellMisc.dbc`, `SpellEffect.dbc`, `SpellIcon.dbc`,
      `SpellCooldowns.dbc`, `SpellLevels.dbc`, `SpellDuration.dbc`, `SpellCastTimes.dbc`, and
      resolved `SpellVisual.db2` `.mdx`/`.mdl` models).
    - `search_visuals` (`weapon_glow` over `SpellItemEnchantment.dbc` -> `ItemVisuals.dbc` ->
      `ItemVisualEffects.dbc` for `heartseed_glow`, `spell_visual` over `SpellVisual.db2` ->
      `SpellVisualKit.db2` -> `SpellVisualEffectName.db2` + `SpellVisualKitModelAttach.db2` +
      sample spells, `visual_effect`, and `icon`).
    - `search_items` (joins `Item.db2`, `Item-sparse.db2`, `ItemDisplayInfo.dbc`, and
      `ItemVisuals.dbc`).
    - `search_creatures` (searches `Creature.db2` plus optional `world.creature_template` and
      `creature_template_model` when `MOP_WORLD_DB` and `MOP_DB_PASSWORD` are set, resolving display
      IDs to `.m2` paths and skins).
    - `search_display_ids` (`creature`, `item`, `gameobject` display IDs, models, skins, icons,
      bounding boxes, and linked entities).
- Updated `TASKS.md` to mark `T-030` as `review`.

## How it was tested
- Ran unit and gamedata integration tests for `tools/dbc/`:
  - `python -m unittest discover -s tools/dbc -v` (4/4 tests passing, verifying all 28 registered
    schemas against `I:/UltimateMoP/gamedata/dbc` and `enUS/*.db2`).
- Ran unit and MCP stdio + gamedata integration tests for `tools/content-search/`:
  - `python -m unittest discover -s tools/content-search -v` (3/3 tests passing).
- Ran CLI smoke checks for `search_visuals` (`weapon_glow`), `search_spells`, and `search_creatures`
  against `world_gemini` and `I:/UltimateMoP/gamedata/dbc`.

## Decisions made
- Separated `tools/dbc/` (reusable binary `WDBC`/`WDB2` reader and schema definitions) from
  `tools/content-search/` (`search_engine.py` core + `server.py` MCP stdio/CLI adapter) per
  `AGENTS.md` modular design rules.
- Made `MOP_WORLD_DB` optional in `tools/content-search/`: all 5 MCP tools work offline purely from
  `MOP_DBC_DIR` (`I:/UltimateMoP/gamedata/dbc`), while setting `MOP_WORLD_DB` and `MOP_DB_PASSWORD`
  enriches `search_creatures` with all 53,517 `creature_template` + `creature_template_model` rows.

## For the other agent
- No shared C++ or SQL hotspots touched.
- For Heartseed weapon glows (`heartseed_glow`) and proc visuals (`Heartseed.Proc.SpellId`), run:
  - `python tools/content-search/server.py --cli search_visuals --kind weapon_glow --query "<keyword>"`
  - `python tools/content-search/server.py --cli search_visuals --kind spell_visual --query "<keyword>"`

## Next
- Ready for review on `T-030` (`gemini/T-030-dbc-content-search`).

## Map additions
- `tools/dbc/`: shared MoP 5.4.8 `WDBC` and `WDB2` reader library and schemas (`T-030`).
- `tools/content-search/`: MCP stdio server and CLI for searching spells, visuals, weapon glows,
  items, creatures, and display IDs (`T-030`).
