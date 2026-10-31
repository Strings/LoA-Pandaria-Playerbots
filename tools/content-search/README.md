# Content Search MCP Server (`tools/content-search/`)

Standalone Python 3.9+ stdio MCP server and CLI tool for searching MoP 5.4.8 (build 18414) spells,
spell visuals, weapon glow enchants, icons, items, creatures, and display IDs (`T-030`). Built on top
of the shared DBC/DB2 reader library in `tools/dbc/`.

## Purpose

- Lets agents and developers find client and server content IDs without launching the client or
  writing ad-hoc binary parsers:
  - **Heartseed visuals:** weapon glow enchantments (`SpellItemEnchantment.dbc` -> `ItemVisuals.dbc`
    -> `ItemVisualEffects.dbc` `.mdx` models), proc spells and spell visuals (`SpellVisual.db2` ->
    `SpellVisualKit.db2` -> `SpellVisualEffectName.db2` + `SpellVisualKitModelAttach.db2`), and icons
    (`SpellIcon.dbc`).
  - **Items & appearances:** item entries (`Item.db2` + `Item-sparse.db2`), icons, 3D model/texture
    names, item visual IDs (`ItemDisplayInfo.dbc`), and item spells.
  - **Creatures & display IDs:** creature entries (`Creature.db2` plus optional
    `world.creature_template`), creature display IDs (`CreatureDisplayInfo.dbc`), `.m2` model paths
    and scales (`CreatureModelData.dbc`), and GameObject display IDs (`GameObjectDisplayInfo.dbc`).

## MCP Stdio Interface

Start with:

```powershell
python tools/content-search/server.py
```

Negotiates MCP protocol `2024-11-05` over newline-delimited JSON-RPC 2.0 (`initialize`, `ping`,
`tools/list`, `tools/call`).

### Exposed MCP Tools

1. `search_spells(query?, spell_id?, effect?, aura?, school_mask?, icon?, visual_id?, has_visual?, limit=20)`
   - Returns matching spells with name, rank, description, tooltip, school mask, icon ID/path,
     `spell_visual_ids`, resolved `.mdx`/`.mdl` visual effect models, cast time, duration, cooldowns,
     levels, and `SpellEffect` rows.
2. `search_visuals(query?, kind="all", visual_id?, kit_id?, effect_id?, enchant_id?, item_visual_id?, limit=20)`
   - `kind`:
     - `"weapon_glow"`: `SpellItemEnchantment.dbc` rows with a visible `aura_id` (`ItemVisuals.dbc`),
       returning `enchant_id`, `description`, `item_visual_id`, and `.mdx` glow models (for
       `heartseed_glow`).
     - `"spell_visual"`: `SpellVisual.db2` rows with resolved stage kits (`precast`, `cast`, `impact`,
       `state`, `channel`, etc.), missile models, `.mdx` paths, and sample spells using that visual.
     - `"visual_effect"`: `SpellVisualEffectName.db2` `.mdx`/`.mdl` models and scales.
     - `"icon"`: `SpellIcon.dbc` icon paths.
3. `search_items(query?, item_id?, display_id?, item_class?, item_subclass?, inventory_type?, quality?, min_ilvl?, max_ilvl?, min_req_level?, max_req_level?, has_item_visual?, limit=20)`
   - Joins `Item.db2`, `Item-sparse.db2`, and `ItemDisplayInfo.dbc`.
4. `search_creatures(query?, creature_id?, display_id?, creature_type?, family?, min_level?, max_level?, rank?, limit=20)`
   - Searches `Creature.db2` (and `world.creature_template` when `MOP_WORLD_DB` and
     `MOP_DB_PASSWORD` are set), resolving display IDs to `.m2` model paths and skins.
5. `search_display_ids(query?, display_id?, kind="all", model_id?, has_item_visual?, limit=20)`
   - `kind` in `("all", "creature", "item", "gameobject")`: searches `CreatureDisplayInfo.dbc` (+
     `CreatureModelData.dbc`), `ItemDisplayInfo.dbc`, and `GameObjectDisplayInfo.dbc`.

## CLI Usage

```powershell
# Find weapon glow enchants for Heartseed stages/paths:
python tools/content-search/server.py --cli search_visuals --kind weapon_glow --query "jade"

# Find lightning or sha spell visuals and sample spells that use them:
python tools/content-search/server.py --cli search_visuals --kind spell_visual --query "lightning" --limit 5

# Inspect a specific spell:
python tools/content-search/server.py --cli search_spells --id 16614

# Search creature display IDs by model or skin keyword:
python tools/content-search/server.py --cli search_display_ids --kind creature --query "celestial" --limit 10
```

## Configuration (Environment Variables)

| Variable | Default | Purpose |
|---|---|---|
| `MOP_DBC_DIR` | `I:/UltimateMoP/gamedata/dbc` | Directory containing `.dbc` and `enUS/*.db2` files |
| `MOP_DBC_LOCALE` | `enUS` | Locale subfolder for `.db2` files |
| `MOP_WORLD_DB` | `""` | Optional world DB name (`world_gemini`, etc.) to include `creature_template` in `search_creatures` |
| `MOP_DB_HOST` / `MOP_DB_PORT` / `MOP_DB_USER` / `MOP_DB_PASSWORD` | `127.0.0.1` / `3306` / `ultimatemop` | Optional MySQL connection settings when `MOP_WORLD_DB` is set |
| `MOP_MYSQL_BIN` | `""` | Optional directory containing `mysql` CLI binary |

## Unit Tests

```powershell
python -m unittest discover -s tools/content-search -v
```
