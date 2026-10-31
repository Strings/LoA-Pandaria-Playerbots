# Repo map

Where things live, so you don't have to re-explore the repo every session. Paths are from the
`playerbot` branch as of 2026-10-30. Add to this file through a status fragment when you find
something worth knowing.

## Top level

| Path | What |
|---|---|
| `src/server/game/` | The core: entities, spells, maps, instances, loot, battlefields, handlers |
| `src/server/scripts/` | Content scripts by continent, plus `Custom/`, `Commands/`, `World/`, `Spells/` |
| `src/server/worldserver/`, `src/server/authserver/` | Server executables and `worldserver.conf.dist` |
| `src/server/database/`, `src/server/shared/` | DB layer and shared utilities |
| `src/tools/` | Map, vmap and mmap extractors |
| `modules/mod_playerbots/` | The playerbots port |
| `sql/` | `base/` (full schemas), `updates/{world,characters,master}/`, `old/`, `archive/` |
| `contrib/headless-client/` | Python 5.4.8 protocol client for automated tests (no graphics) |
| `contrib/SkyFire Patcher/548/` | Arctium client exe patcher (pattern find and replace); the interface signature patch will go here |
| `dep/` | Third-party libraries (StormLib for MPQ, g3dlite, Detour and Recast, and others) |
| `.github/workflows/` | CI: `linux_gcc.yml`, `linux_clang.yml`, `macos-arm-build.yml`, `beta-prerelease.yml` |

## Core (`src/server/game/`)

| Path | Use it for |
|---|---|
| `Entities/Unit/Unit.cpp` | Movement speed, auras, combat (mount speed quick win) |
| `Entities/Player/Player.cpp` | Player state, equipment, skills, enchant slots (jewel) |
| `Instances/InstanceSaveMgr.cpp` | Instance saves and reset timing (2-day raid resets) |
| `Loot/LootMgr.cpp`, `Handlers/LootHandler.cpp` | Loot generation and handling |
| `Spells/Spell.cpp` | Spell casting; spell scripts hook in via `SpellScript` |
| `LuaEngine/` | Eluna Lua engine (off by default; `Eluna.*` keys in `worldserver.conf.dist`); prototype content in `lua_scripts/` |
| `Battlefield/` | `BattlefieldMgr`; `Zones/BattlefieldWG.cpp` (Wintergrasp), `Zones/BattlefieldTB.cpp` (Tol Barad) |

## Scripts (`src/server/scripts/`)

| Path | What |
|---|---|
| `Custom/` | Our scripts go here. Already contains `PlayerStartScript.cpp`, `solocraft.cpp`, `npc_teleport.cpp`, `custom_items.cpp` and others; check them before writing similar code |
| `ScriptLoader/ScriptLoader.cpp` | Script registration (shared hotspot) |
| `Pandaria/` | MoP raids, dungeons, zones, world bosses; `Scenarios/` has 17 scenarios including `FallOfTheramoreAlliance` and `FallOfTheramoreHorde` (jewel intro candidates) |
| `EasternKingdoms/`, `Kalimdor/`, `Outland/`, `Northrend/`, `Maelstrom/` | Older content, including every Classic to Cata raid |
| `OutdoorPvP/` | Hellfire towers (HP), Halaa (NA), Silithus (SI), Terokkar (TF), Zangarmarsh (ZM) |
| `World/BrawlersGuild/` | Brawler's Guild (base for the Stupid Ladder) |
| `World/transmog_scripts.cpp`, `../game/CustomTransmogrification/` | NPC transmog module (Collections will build on it) |
| `Toys/` | Effect scripts for individual toy items (not a toy box) |

## Playerbots (`modules/mod_playerbots/`)

| Path | What |
|---|---|
| `config/playerbots.conf.dist` | All `AiPlayerbot.*` settings (shared hotspot) |
| `src/AI/` | `PlayerbotAI`, including `AllowActivity` (dormant/wake hook point) |
| `src/Manager/` | Random bot manager, `BattlefieldBots` (Wintergrasp and Tol Barad joining) |
| `src/strategy/` | Strategies, actions, triggers, values; `Classes/` per class (monk included); `dungeons/` per instance (all MoP dungeons; no raids); `RPG/` (`NewRpgStrategy`, RPG status weights) |
| `src/Scenario/` | `.botscenario` GM test commands |
| `src/Travel/` | Travel manager and world positions |
| `data/sql/playerbots/updates/db_playerbots/` | Playerbots DB updates |

## Build options (`cmake/options.cmake`)

`SERVERS`, `SCRIPTS`, `PLAYERBOTS`, `ELUNA` on by default; `TOOLS` off; `USE_COREPCH` and `USE_SCRIPTPCH` on; `WITH_SANITIZER` and `WITH_COREDEBUG` off. No unit test option yet (T-008).

## Data and runtime

- Four databases: `auth`, `characters`, `world`, `playerbots` (one set per agent; see AGENTS.md).
- Game data folder: `I:\UltimateMoP\gamedata\` (gitignored) with `dbc`, `maps`, `vmaps`, `mmaps`,
  `cameras` extracted from the 5.4.8 enUS client. Shared and read-only.
- Game client: `I:\UltimateMoP\mop-clean\` (5.4.8.18414 enUS, 24 GB, gitignored, read-only).
  `_Wow.exe` and `_Wow-64.exe` skip file verification (needed for custom patch MPQs); `WTF/Config.wtf`
  still points at Tauri's realmlist; the connection patch comes from `contrib/SkyFire Patcher/548`.
- `worldserver.conf` keys: `*DatabaseInfo`, `DataDir`, `LogsDir`, `SOAP.*`, `Updates.EnableDatabases`
  (default 0).
- `playerbots.conf` sits next to `worldserver.conf`.

## Design reference

`docs/DESIGN.md` is an export of the design doc (last exported 2026-10-31). Section names match the
roadmap phases in `TASKS.md`. Items marked provisional are suggestions until a task says otherwise.
"Roadmap and task list" near the end holds the decided rules (priority rule, build for the end state,
accepted interim states) and the milestones.
