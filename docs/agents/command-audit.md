# Custom Commands and RBAC Permissions Audit

Comprehensive audit of all chat commands registered under `src/server/scripts/Custom/`, checking RBAC permission IDs, assignments in `docs/ID_RANGES.md`, and SQL definitions in `sql/updates/master/auth/`.

> **Task Reference (T-068):** Report only. Do not edit code or SQL files.

---

## 1. Executive Summary

- **Total Custom CommandScript implementations audited:** 12 files
- **Total distinct custom commands / subcommands:** 33 commands across 10 top-level commands
- **Dedicated custom RBAC IDs defined in `docs/ID_RANGES.md`:** 11 IDs across 3 blocks
- **Auth SQL migration coverage:** 100% of dedicated custom RBAC IDs have idempotent SQL migration rows in `sql/updates/master/auth/`
- **ID Collisions:** **0** (No collisions detected across any custom IDs)
- **Architectural Gaps Identified:** 5 systems borrow core Blizzard/TrinityCore stock RBAC permission IDs (`830`, `811`, `742`) instead of assigning dedicated IDs within their subsystem's reserved block in `docs/ID_RANGES.md`.

---

## 2. Command Inventory and RBAC Matrix

The table below catalogs every chat command added under `src/server/scripts/Custom/`, the RBAC permission ID used in C++, its reserved block in `docs/ID_RANGES.md`, and the corresponding SQL migration file in `sql/updates/master/auth/`.

| File | Command(s) | RBAC ID in C++ | Constant Name / Core Ref | ID Range in `docs/ID_RANGES.md` | Auth SQL Migration | Target Role | Status |
|---|---|---|---|---|---|---|---|
| `Heartseed/heartseed_scripts.cpp` | `.heartseed` (`rank`, `growth`, `flag`, `cap`, `feed`, `essence`, `traits`, `major`, `reload`, `shrines`) | 90000 | `RBAC_PERM_COMMAND_HEARTSEED` | 90000–90999 (Heartseed) | `2026_10_31_claude_02_heartseed_command.sql` | 197 (GM) | Validated |
| `Heartseed/heartseed_scripts.cpp` | `.seed` | 90001 | `RBAC_PERM_COMMAND_SEED` | 90000–90999 (Heartseed) | `2026_10_31_claude_11_seed_command.sql` | 199 (Player) | Validated |
| `GMTools/Checkpoints/checkpoint_scripts.cpp` | `.checkpoint` (`list`, `explain`, `apply`, `reload`) | 91000 | `RBAC_PERM_COMMAND_CHECKPOINT` | 91000–91999 (GM tools) | `2026_10_31_claude_13_gmtools_commands.sql` | 197 (GM) | Validated |
| `GMTools/Checkpoints/checkpoint_scripts.cpp` | `.kit` (`list`, `explain`, `apply`) | 91001 | `RBAC_PERM_COMMAND_KIT` | 91000–91999 (GM tools) | `2026_10_31_claude_13_gmtools_commands.sql` | 197 (GM) | Validated |
| `GMTools/Checkpoints/checkpoint_scripts.cpp` | `.gmsuite` (`refresh`, `sync`) | 91002 | `RBAC_PERM_COMMAND_GMSUITE` | 91000–91999 (GM tools) | `2026_10_31_claude_13_gmtools_commands.sql` | 197 (GM) | Validated |
| `GMTools/Clock/gmtools_clock.cpp` | `.clock` (`info`, `speed`, `jump`, `reset`) | 91003 | `RBAC_PERM_COMMAND_CLOCK` | 91000–91999 (GM tools) | `2026_10_31_claude_18_gmtools_t039.sql` | 197 (GM) | Validated |
| `GMTools/Scenarios/gmtools_scenarios.cpp` | `.scenario` (`list`, `explain`, `load`) | 91005 | `RBAC_PERM_COMMAND_SCENARIO` | 91000–91999 (GM tools) | `2026_10_31_claude_18_gmtools_t039.sql` | 197 (GM) | Validated |
| `GMTools/Spectate/gmtools_spectate.cpp` | `.spectate` (`stop`, `<name>`) | 91006 | `RBAC_PERM_COMMAND_SPECTATE` | 91000–91999 (GM tools) | `2026_10_31_claude_18_gmtools_t039.sql` | 197 (GM) | Validated |
| `Loadouts/loadout_scripts.cpp` | `.loadout` (`list`, `switch`, `new`, `rename`, `delete`, `save`, `sync`, `page put`, `page take`) | 93000 | `RBAC_PERM_COMMAND_LOADOUT` | 93000–93999 (Loadouts) | `2026_11_01_claude_01_loadout_command.sql` | 199 (Player) | Validated |
| `Loadouts/loadout_scripts.cpp` | `.loadout inspect` | 93001 | `RBAC_PERM_COMMAND_LOADOUT_GM` | 93000–93999 (Loadouts) | `2026_11_01_claude_01_loadout_command.sql` | 197 (GM) | Validated |
| `ReagentBank/reagentbank_scripts.cpp` | `.reagent` (`list`, `deposit`, `take`, `sync`) | 93002 | `RBAC_PERM_COMMAND_REAGENT` | 93000–93999 (Detached storage) | `2026_11_01_claude_03_reagent_command.sql` | 199 (Player) | Validated |
| `ReagentBank/reagentbank_scripts.cpp` | `.reagent inspect`, `.reagent add` | 93003 | `RBAC_PERM_COMMAND_REAGENT_GM` | 93000–93999 (Detached storage) | `2026_11_01_claude_03_reagent_command.sql` | 197 (GM) | Validated |
| `MountedSpeed/MountedSpeed.cpp` | `.mountspeed` | 830 | `rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG` | *None (Stock Core ID)* | *None (Core stock)* | 197 (GM) | Borrowed |
| `Reload/CustomReload.cpp` | `.reload custom [table]`, `.rng [seed/status/off]` | 830 | `rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG` | *None (Stock Core ID)* | *None (Core stock)* | 197 (GM) | Borrowed |
| `Reload/CustomReload.cpp` | `.reload [table]` | 811 | `rbac::RBAC_PERM_COMMAND_RELOAD` | *None (Stock Core ID)* | *None (Core stock)* | 197 (GM) | Borrowed |
| `Respec/Respec.cpp` | `.respec` (`info`, `reset`, `learn`, `cast`) | 830 | `rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG` | *None (Stock Core ID)* | *None (Core stock)* | 197 (GM) | Borrowed |
| `SoloConfig/SoloConfig.cpp` | `.solo` (`info`, `fishskill`, `award`) | 830 | `rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG` | *None (Stock Core ID)* | *None (Core stock)* | 197 (GM) | Borrowed |
| `Capture/capture_scripts.cpp` | `.idea`, `.bug`, `.capture list`, `.capture show` | 742 | `rbac::RBAC_PERM_COMMAND_TICKET` | *None (Stock Core ID)* | *None (Core stock)* | 198 (Mod) | Borrowed |

*(Note: `91004` is allocated in GM tools block and registered in `2026_10_31_claude_18_gmtools_t039.sql` for `.bot inspect`, which is implemented in `modules/mod_playerbots/src/Inspect/InspectBot.cpp`).*

---

## 3. Analysis of Dedicated Custom Ranges

According to [`docs/ID_RANGES.md`](file:///I:/UltimateMoP/docs/ID_RANGES.md), custom RBAC permissions use the lower portion of each system's assigned gossip/system ID block:

### Block 90000–90999: Heartseed
- `90000` — `Command: heartseed` (GM inspection and tuning; role 197)
- `90001` — `Command: seed` (Player path-choice window; role 199)
- *Next available in block:* `90002`

### Block 91000–91999: GM Tools
- `91000` — `Command: checkpoint` (role 197)
- `91001` — `Command: kit` (role 197)
- `91002` — `Command: gmsuite` (role 197)
- `91003` — `Command: clock` (role 197)
- `91004` — `Command: bot (inspect)` (role 197)
- `91005` — `Command: scenario` (role 197)
- `91006` — `Command: spectate` (role 197)
- *Next available in block:* `91007`

### Block 93000–93999: Loadouts and Detached Storage
- `93000` — `Command: loadout` (Player loadout manager; role 199)
- `93001` — `Command: loadout inspect` (GM loadout inspector; role 197)
- `93002` — `Command: reagent` (Player reagent bank; role 199)
- `93003` — `Command: reagent inspect, add` (GM reagent inspector; role 197)
- *Next available in block:* `93004`

---

## 4. Gaps and Inconsistencies

### 4.1. Systems Borrowing Core Permissions ("Borrowed" Status)
Five custom scripts reuse existing stock TrinityCore/Skyfire permissions rather than registering their own scoped permissions in their system's range:
1. **`Respec.cpp`:** Uses `rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG` (830). Ideally, respec commands should have their own permission (e.g. `Command: respec`).
2. **`SoloConfig.cpp`:** Uses `rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG` (830). Should eventually use a dedicated GM ID under its own reserved system block.
3. **`MountedSpeed.cpp`:** Uses `rbac::RBAC_PERM_COMMAND_RELOAD_CONFIG` (830) for `.mountspeed`.
4. **`CustomReload.cpp`:** Reuses `RBAC_PERM_COMMAND_RELOAD_CONFIG` (830) and `RBAC_PERM_COMMAND_RELOAD` (811). This is reasonable since it extends `.reload`, but `.rng` could arguably have a standalone permission in the testing/tools block.
5. **`Capture/capture_scripts.cpp`:** Uses `rbac::RBAC_PERM_COMMAND_TICKET` (742) for `.idea`, `.bug`, and `.capture`. Note: 742 is linked only to role 198 (moderator commands), meaning standard player accounts cannot use `.bug` or `.idea`. Being resolved via BUG-004 (dedicated player permission for `.idea`/`.bug`, GM permission for `.capture`).

### 4.2. ID Collisions
- **No collisions found.**
- All IDs assigned in `sql/updates/master/auth/*.sql` match the constants defined in C++ headers/source files exactly and respect the boundary blocks documented in `docs/ID_RANGES.md`.

---

## 5. Recommendations

1. **Keep Borrowed Permissions for Generic Extensions:**
   Extending `.reload` via `CustomReload.cpp` using `RBAC_PERM_COMMAND_RELOAD` is clean and standard practice.
2. **Assign Dedicated Permissions when Subsystems Solidify:**
   If `.respec` or `.solo` need player-facing or fine-grained GM delegation in future phases, allocate IDs in the 94000+ range or the GM Tools 91000 range and add matching auth migration files.
3. **Update `docs/ID_RANGES.md` Documentation:**
   Add explicit mention of the assigned RBAC IDs for Loadouts and Reagent Bank (`93000-93003`) in the notes section of `docs/ID_RANGES.md` to keep the reference documentation fully synchronized.
