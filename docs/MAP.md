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
| `addons/` | Client addons we ship (`addons/GMSuite/`, `addons/Capture/`) |
| `tickets/` | Captured in-game `/idea` and `/bug` feedback tickets |
| `tools/` | Developer tooling (build, benchmarks, live MCP, DBC reader, content search, SQL validator, twink tuning) |
| `tests/` | Pure logic unit test suites and regression scenarios |
| `contrib/headless-client/` | Python 5.4.8 protocol client for automated tests (no graphics) |
| `contrib/SkyFire Patcher/548/` | Arctium client exe patcher (pattern find and replace); the interface signature patch will go here |
| `dep/` | Third-party libraries (StormLib for MPQ, g3dlite, Detour and Recast, and others) |
| `.github/workflows/` | CI: `linux_gcc.yml`, `linux_clang.yml`, `macos-arm-build.yml`, `beta-prerelease.yml` |

## Core (`src/server/game/` and `src/common/`)

| Path | Use it for |
|---|---|
| `Accounts/AccountMgr.cpp` | Account creation, hashing, and CLI login accounts |
| `Entities/Unit/Unit.cpp` | Movement speed, auras, combat (mount speed quick win) |
| `Entities/Unit/MountedSpeed.h`, `MountedSpeed.cpp` | Mount speed multiplier calculation and cached rate config |
| `Entities/Player/Player.cpp` | Player state, equipment, skills, enchant slots (jewel) |
| `Instances/InstanceSaveMgr.cpp` | Instance saves and reset timing (2-day raid resets) |
| `Loot/LootMgr.cpp`, `Handlers/LootHandler.cpp` | Loot generation and handling |
| `Spells/Spell.cpp` | Spell casting; spell scripts hook in via `SpellScript` |
| `Time/CustomClock.h` | Controllable scaled/paused clock for custom systems' timers |
| `World/RespecConfig.h`, `RespecConfig.cpp` | Cached dual-spec toggles, reagent rules, and cost policies |
| `World/SoloConfig.h`, `CurrencyLimits.h` | Solo play switches, weekly currency cap math, and fishing skillups |
| `src/common/Utilities/DeterministicRandom.h`, `RandomSeedState.h` | Deterministic seeded RNG streams for test profiles |
| `LuaEngine/` | Eluna Lua engine (off by default; `Eluna.*` keys in `worldserver.conf.dist`); prototype content in `lua_scripts/` |
| `Battlefield/` | `BattlefieldMgr`; `Zones/BattlefieldWG.cpp` (Wintergrasp), `Zones/BattlefieldTB.cpp` (Tol Barad) |

## Scripts (`src/server/scripts/`)

| Path | What |
|---|---|
| `Custom/` | Our scripts go here. Already contains `PlayerStartScript.cpp`, `solocraft.cpp`, `npc_teleport.cpp`, `custom_items.cpp` and others; check them before writing similar code |
| `Custom/Heartseed/` | The Heartseed living jewel system (stages, feeding, traits, essences) |
| `Custom/GMTools/` | GM test suite (`Checkpoints/`, `Clock/`, `Scenario/`, `Spectate/`, `README.md` overview) |
| `Custom/MountedSpeed/` | `.mountspeed` inspect command and configuration docs |
| `Custom/Respec/` | `.respec` inspect, reset, learn commands |
| `Custom/SoloConfig/` | `.solo` inspect and test commands |
| `Custom/Reload/` | Hot-reload registry and `.reload rng` command |
| `Custom/Capture/` | `.idea`, `.bug`, and `.capture` feedback ticket capture commands |
| `ScriptLoader/ScriptLoader.cpp` | Script registration (shared hotspot) |
| `Pandaria/` | MoP raids, dungeons, zones, world bosses; `Scenarios/` has 17 scenarios including `FallOfTheramoreAlliance` and `FallOfTheramoreHorde` (jewel intro candidates) |
| `EasternKingdoms/`, `Kalimdor/`, `Outland/`, `Northrend/`, `Maelstrom/` | Older content, including every Classic to Cata raid |
| `OutdoorPvP/` | Hellfire towers (HP), Halaa (NA), Silithus (SI), Terokkar (TF), Zangarmarsh (ZM) |
| `World/BrawlersGuild/` | Brawler's Guild (base for the Stupid Ladder) |
| `World/transmog_scripts.cpp`, `../game/CustomTransmogrification/` | NPC transmog module (Collections will build on it) |
| `Toys/` | Effect scripts for individual toy items (not a toy box) |
| `src/server/worldserver/CommandLine/CliRunnable.cpp` | Server console thread and interactive stdin handling |

## Playerbots (`modules/mod_playerbots/`)

| Path | What |
|---|---|
| `config/playerbots.conf.dist` | All `AiPlayerbot.*` settings (shared hotspot) |
| `src/mod_playerbots.cpp` | Playerbots initialization and `playerbots.conf` loader |
| `src/AI/` | `PlayerbotAI`, including `AllowActivity` (dormant/wake hook point) |
| `src/Inspect/` | `.bot` inspection, decision trace (`.bot why`), section registry |
| `src/Manager/` | Random bot manager, `BattlefieldBots` (Wintergrasp and Tol Barad joining) |
| `src/strategy/` | Strategies, actions, triggers, values; `Classes/` per class (monk included); `dungeons/` per instance (all MoP dungeons; no raids); `RPG/` (`NewRpgStrategy`, RPG status weights) |
| `src/Scenario/` | `.botscenario` GM test commands |
| `src/Travel/` | Travel manager and world positions |
| `data/sql/playerbots/updates/db_playerbots/` | Playerbots DB updates |

## Tools and tests (`tools/` and `tests/`)

| Path | What |
|---|---|
| `tools/build/` | Shared build script (`build.ps1`), machine lock, and compiler cache notes |
| `tools/bench/` | Startup profiling and benchmarking tools (`startup_profile.py`) |
| `tools/live-server-mcp/` | MCP server adapter for live worldserver inspection |
| `tools/backup/` | Save safety and database snapshot tooling (`save_safety.py`) |
| `tools/dbc/` | Shared MoP 5.4.8 `WDBC` and `WDB2` reader library and named schemas |
| `tools/content-search/` | Content search MCP server and CLI for spells, visuals, weapon glows, items |
| `tools/sql-validator/` | SQL update linter (filename, CRLF, idempotence, ID ranges, collisions, schemas) |
| `tools/twink-tuning/` | Twink stat-buff SQL generator and bracket scaling math |
| `tools/devenv/` | Development environment profiles and live scenarios (`solo_smoke.json`) |
| `tests/heartseed/` | Unit tests for Heartseed growth and trait rules |
| `tests/mounted_speed/` | Unit tests for mount speed multiplier math |
| `tests/respec/` | Unit tests for dual-spec unlock and cost policies |
| `tests/reload/` | Unit tests for deterministic seeded RNG streams |
| `tests/regression/scenarios/` | Headless client regression scenarios (`custom-reload-rng.json`, etc.) |

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
