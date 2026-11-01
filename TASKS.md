# Tasks

Claim a task by setting **Owner** to your agent name and **Status** to `in progress` in your
branch. Status values: `open`, `in progress`, `review`, `blocked: <question>`, `done`.
**Owns** lists the paths the task may change freely (AGENTS.md: incidental edits elsewhere are fine).

**Priority rule:** cheap work that speeds up the early phases goes first. T-001 blocks everything.
After it, fast iteration (T-005 to T-009, T-035, T-037 to T-040) and the cheap tools (T-032 to
T-034) come before most feature tasks, except the Heartseed track, which is the top priority.

**Build for the end state:** build systems in their final shape; see "Accepted interim states" in
`docs/DESIGN.md` for what is allowed to stay incomplete until later phases.

## v0.1 The jewel run

### Foundation

| ID | Task | Owns | Owner | Status |
|---|---|---|---|---|
| T-001 | Fork LoA, turn this folder into the repo, build with `TOOLS=ON`, extract game data from `mop-clean/` into `gamedata/`, import the four databases and the per-agent sets, start the servers, create worktrees. See `HANDOFF.md` | repo setup, `gamedata/`, `.gitignore` | claude | done |
| T-002 | Dev environment script: create per-agent databases from the dumps, set ports and realmlist, write per-agent conf files | `tools/devenv/` | codex | done |
| T-003 | Baseline `.server info` (median, p95, p99) at 500, 1000 and 2000 bots on the human's play machine; record results in a status fragment | `tools/bench/` | | open |
| T-004 | Player telemetry logging hook (movement, casts, targets, emotes, loot, chat) behind a config key, off by default. Every decision event carries a context snapshot (self, target, surroundings, place and time) in relative values with stable IDs; outcome events and activity-episode tags; versioned schema; movement sampled at a config rate; chat behind its own key; `.telemetry pause|resume|discard|tag <intent>` and automatic pauses (GM mode, GM commands, AFK, idle). Also line of sight and height difference on decision events, cast-cancel point with the target's interrupt state, diminishing-returns state, raw movement flags, emotes and chat as decision events, objective state events, `actor_type` on every row; allies' centroid distance, angle and heading, and allies already on a new target; time since combat ended on decision events, death-to-release time, facing changes kept on movement events (gaze lead). Spec: `docs/DESIGN.md`, "Decision context" under human-like bots | `src/server/game/Telemetry/` | | open |
| T-036 | Server timeline decision (human), then a short doc of the core loops per time scale (minute, session, week, life) drafted by an agent | `docs/TIMELINE.md`, `docs/LOOPS.md` | human | open |
| T-037 | Save safety: nightly snapshots of all databases with rotation (file-copy restore), and a migration check that runs new custom-table SQL against a copy of the real save first | `tools/devenv/`, `tools/backup/` | codex | done |

### Fast iteration (Phase 0a, do first)

| ID | Task | Owns | Owner | Status |
|---|---|---|---|---|
| T-005 | Test profile config: bots off or minimal, smaller logs, grid preloading off, short timers; bot load profiles (none, local, scale) | `tools/devenv/` | codex | done |
| T-006 | Startup profiling script that ranks the server's loader timings from the log; report the top costs in a fragment | `tools/bench/` | codex | done |
| T-007 | Fixture characters (levels 1, 30, 60, 85, 90 with gear), pre-generated bot snapshot, file-copy database restore | `tools/devenv/`, `sql/fixtures/` | | open |
| T-008 | Unit test framework in CMake behind an option, with one example test; document how to add tests | `tests/`, `CMakeLists.txt` | codex | done |
| BUG-001 | Guard unknown phase groups in Creature::Create so .npc add cannot dereference null | `src/server/game/Entities/Creature/Creature.cpp` | codex | done |
| BUG-003 | Investigate aura-removal crash after deleting a newly added training dummy (entry 31144) | `src/server/game/Spells/Auras/` | | open |
| BUG-002 | Accept quoted source columns in migration checks while rejecting client SOURCE directives | `tools/backup/` | codex | done |
| T-009 | Enable Eluna in the dev and test profiles; `lua_scripts/` layout and a README for prototypes | `lua_scripts/`, `tools/devenv/` | codex | done |
| T-035 | Headless-client regression suite runner with assertions; one example scenario | `tests/regression/`, `contrib/headless-client/` | codex | done |
| T-038 | Hot reload framework for custom tables (`.reload <table>`), settable global RNG seed for tests. Built by Codex, finished and merged by Claude; faster links moved to T-060 | `src/server/scripts/Custom/Reload/` | codex, claude | done |
| T-060 | Faster builds: lld-link where it works with MSVC (split out of T-038), and make sccache useful: it caches nothing today because every compile uses the precompiled header (`/Fp`, all 1190 calls non-cacheable). Measure PCH on and no cache against PCH off and cached, and a unity build for the playerbots module | build config, `tools/build/` | | open |
| T-039 | GM tools, first set: server clock speed-up and jump, `.scenario load <name>` on top of `.botscenario`, `.bot why` (last decision with candidate scores), `.bot traits/memory/skill`, invisible spectate. Overlays and force-any-system commands grow with each system | claude | done |
| T-040 | `/idea` and `/bug` capture: save a note with position, target, nearby bots and their last decisions, active systems and a screenshot path as a ticket file agents can pick up | gemini, claude | done |

### Quick wins

| ID | Task | Owns | Owner | Status |
|---|---|---|---|---|
| T-010 | Double mounted movement speed via a config multiplier (default 2.0) in the core speed update; check ghost wolf and travel forms are untouched for now | codex | done |
| T-011 | Apprentice Riding and a basic faction-appropriate mount at character creation; four 10-slot bags in the bag slots | `sql/updates/world/`, `src/server/scripts/Custom/PlayerStartScript.cpp` | | open |
| T-012 | Raise rare mount drops to 10%: list every rare-drop mount item and update loot tables | `sql/updates/world/` | codex | done |
| T-013 | Raid resets every 2 days instead of weekly; keep in-progress instances until finished or abandoned | `src/server/game/Instances/` | | open |
| T-014 | All professions: raise `MaxPrimaryTradeSkill` (confirm the key exists); addon button bar for professions past the second | config, `addons/ProfessionBar/` | | open |
| T-015 | No item binding: config-keyed hook that skips binding, keeping quest items and custom progression items bound; one-time soulbound-flag cleanup SQL; tooltip fix via item hotfixes. Check the transmog interaction | `src/server/scripts/Custom/NoBinding/`, `sql/updates/` | | open |
| T-016 | Free talent and glyph changes (no gold, no reagents); spec switching from level 10 | `src/server/scripts/Custom/Respec/` | codex | done |
| T-017 | Server-side spec loadouts (spec, talents, glyphs, action bars, Heartseed paths), one per spec by default plus named extras; addon picker | `src/server/scripts/Custom/Loadouts/`, `addons/Loadouts/` | | open |
| T-018 | Gear pages per loadout: detached storage, page swap through the normal equip path, addon page panel | `src/server/scripts/Custom/Loadouts/`, `addons/Loadouts/` | | open |
| T-019 | Config quick wins: cross-faction play (`AllowTwoSide.*`), all flight paths, guaranteed skill-ups, no weekly currency caps, lighter death penalties. Confirm each key exists in the fork | codex | done |
| T-041 | Script and data quick wins: guild perks for everyone, unrestricted transmog (mixed types), bigger stacks via item hotfixes, free character services NPC, shorter hearthstone cooldown | `src/server/scripts/Custom/SoloQoL/`, `sql/updates/` | | open |
| T-042 | 36-slot bags: expensive vendor (gold sink); hooks to plant them in rare loot later | `sql/updates/world/` | | open |
| T-043 | Reagent bank in detached storage, with crafting drawing from it | `src/server/scripts/Custom/ReagentBank/` | | open |
| T-044 | Free XP on/off toggle (command now, settings panel later); no separate battleground queue for XP-off characters | `src/server/scripts/Custom/Twinks/` | | open |
| T-045 | **Priority:** twink gear buffs above heirlooms via item hotfixes, per bracket and breakpoint (compile the current best non-heirloom items first) | `sql/updates/world/`, `docs/twinks.md` | codex | done |
| T-046 | Dungeon finder for outleveled dungeons: solo teleport at 20+ levels over the dungeon's level (config key). Bot groups wait for the instance pool (Phase 5) | `src/server/game/DungeonFinding/` hooks, `src/server/scripts/Custom/` | | open |
| T-047 | Multi-type tracking test: combine tracking bitmasks behind a config key; test whether the client draws several types and shows several checkmarks | `src/server/scripts/Custom/Tracking/` | | open |

### The Heartseed (Phase 0b, top priority)

| ID | Task | Owns | Owner | Status |
|---|---|---|---|---|
| T-020 | Heartseed state table (rank, Growth, path loadouts per spec, essences, story flags); equip script with hidden enchant in a free slot and stage glow. IDs from `docs/ID_RANGES.md` | `src/server/scripts/Custom/Heartseed/`, `sql/updates/characters/` | claude | done |
| T-021 | Trait table as data rows; core passive scaling with weapon item level or DPS; first major choice at 10 | `src/server/scripts/Custom/Heartseed/`, `sql/updates/world/` | claude | done |
| T-022 | Growth from feeding replaced weapons, two essence types, rank cap by character level. Pacing numbers are placeholders until v0.5 | `src/server/scripts/Custom/Heartseed/` | claude | done |
| T-023 | Gossip menu for path choices (prototype in Lua once T-009 lands) | `src/server/scripts/Custom/Heartseed/`, `lua_scripts/heartseed/` | claude | done |
| T-024 | GM commands: rank, Growth, story flags | `src/server/scripts/Custom/Heartseed/` | claude | done |
| T-025 | Granting quest at level 1 as the canonical "has a Heartseed" marker. The full Theramore intro scenario comes later and completes the same quest; it waits on T-036 (timeline) and the Horde figure decision | `src/server/scripts/Custom/Heartseed/`, `sql/updates/world/` | claude | done |
| T-026 | Story flags set by simple hooks: first dungeon, reached Pandaria, first world boss | `src/server/scripts/Custom/Heartseed/` | claude | done |
| T-049 | Heartseed account-wide (DESIGN.md, One seed per account): move the marker, rank, Growth, essence collection and morph gallery to account tables; keep loadouts, essence slotting and body story flags per character; effective rank = min(account rank, the character's own cap); alt playerbots earn no Growth. Migration of existing per-character rows, checked against a copy of a real save (T-037). Fresh-seed challenge and bot recognition wait for their systems | `src/server/scripts/Custom/Heartseed/`, `sql/updates/characters/` | claude | done |
| T-050 | Commit the T-045 twink buff generator: regenerates `2026_10_31_codex_00_twink_gear.sql` (or a successor) with the target percentage, stat weights and brackets as arguments, so twink tuning can change after playtests without redoing the research | `tools/twink-tuning/` | gemini | done |

### Tooling (v0.1, cheap first)

| ID | Task | Owns | Owner | Status |
|---|---|---|---|---|
| T-032 | Live server MCP: thin wrapper over SOAP and logs (bots near a position, bot state, `.botscenario`, `.server info`, log tail, GM commands). Standalone module | `tools/live-server-mcp/` | codex | done |
| T-033 | Crash triage: read crash dumps and core logs, map to source, suggest the likely commit | `tools/crash-triage/` | gemini | done |
| T-034 | SQL validator: lint new SQL against schema and references (givers, items, ID collisions) | `tools/sql-validator/` | gemini | done |
| T-031 | Balance sim: `.botscenario` dummy room that reports DPS for a given Heartseed loadout | `tools/balance-sim/`, `modules/mod_playerbots/src/Scenario/` | | open |
| T-030 | Shared DBC reader library, then content search on top of it (spells, visuals, items, creatures, display IDs) as an MCP tool | `tools/dbc/`, `tools/content-search/` | gemini | done |
| T-048 | GM test suite for human playtests. **Checkpoint templates** as data rows, applied in one command (`.checkpoint apply <name>`, `.checkpoint list`, `.checkpoint explain <name>`): level, gear set, spells and talents, Heartseed rank, Growth, majors, essences and story flags, legacy projects done, quests, reputations, location. First checkpoints at each Heartseed major tier (10, 25, 40, 55, 70, 85) and at the first and second legendary unlocks (placeholder levels until the pacing pass). **Twink kits** per bracket from T-045's gear. **Ability tester:** learn a spell, reset cooldowns, refill power, set level. **Addon panel** (no client patch) listing checkpoints and kits with one-click apply; the server does the work, the addon only sends commands. Each later system adds its fields to the template. Builds on T-007 fixtures and T-039 GM tools | `src/server/scripts/Custom/GMTools/Checkpoints/`, `sql/updates/world/`, `addons/GMSuite/` | claude | done |

## Later (parked, not v0.1)

| ID | Task | Owns | Owner | Status |
|---|---|---|---|---|
| T-059 | PvP sparring battlegrounds (DESIGN.md, Human-like bots, PvP sparring and negative examples): a test version of each battleground plus an open arena, player vs a single bot, started with `.spar`; telemetry from the player as the target and from the scripted bot as negative examples, tagged by source. Waits on the telemetry hook (Phase 13), start when bot behavior work begins | `src/server/scripts/Custom/Sparring/`, `sql/updates/world/` | | parked |

## Flash queue

Small, checkable tasks for the Flash model (Antigravity, `gemini` worktree, branch prefix and databases). Work
top to bottom. Rules:

- One task per branch and PR (`gemini/<task-id>-<slug>`), one commit per logical step.
- Run the task's check before opening the PR and paste its output into the PR description.
- Touch only the task's paths. Never touch Heartseed, `Custom/Reload`, core server files or the shared hotspots
  (AGENTS.md) unless the task says so.
- If the check fails twice, or the task needs a decision it doesn't state, set it to `blocked` here with a one-line
  question and move on to the next task.
- Never merge: Claude Code reviews every PR first.

| ID | Task | Owns | Check | Owner | Status |
|---|---|---|---|---|---|
| T-051 | Default data path from the repo, not `I:/`: `tools/dbc/reader.py`, `tools/content-search/server.py` and both tests resolve `<repo>/gamedata/dbc` from `__file__` when `MOP_DBC_DIR` is unset | `tools/dbc/`, `tools/content-search/` | both unittest suites pass; no `I:/` left (grep) | flash | open |
| T-052 | SQL validator reads Spell.dbc through `tools/dbc` instead of its own `load_wdbc_first_uint32_ids`; same results | `tools/sql-validator/` | validator unittests pass; validator output on `sql/updates/world` unchanged (diff before and after) | flash | open |
| T-053 | Run the SQL validator over every agent update in `sql/updates/` and write the findings to `docs/agents/sql-audit.md` (file, finding, suggested fix). Report only: never edit an update that has been applied | `docs/agents/sql-audit.md` | the report lists every file checked | flash | open |
| T-054 | Regression scenario for the Heartseed GM commands (`.heartseed grant`, `info`, `cap`, `rank`, `growth +N`, `flag set/clear`, `essence add/slot`, `revoke`), expecting the messages in `status/claude-T-049.md` and the Heartseed README | `tests/regression/scenarios/` | `python tests/regression/run.py` passes it against the gemini server | flash | open |
| T-055 | Regression scenario for checkpoints: `.checkpoint list`, `.checkpoint explain hs-10`, `.checkpoint apply hs-10` on a fresh character, then `.heartseed info` shows level 10 and the seed | `tests/regression/scenarios/` | as T-054 | flash | open |
| T-056 | Bring `docs/MAP.md` up to date from the "Map additions" of every status fragment in `status/` | `docs/MAP.md` | every path listed in a fragment's Map additions appears in MAP.md and exists | flash | open |
| T-057 | Config key audit: every key read by `sConfigMgr` under `src/server/scripts/Custom/` is documented in `worldserver.conf.dist`. Write missing keys to `docs/agents/config-audit.md`; do not edit the `.conf.dist` | `docs/agents/config-audit.md` | the report lists each key with its file and line | flash | open |
| T-058 | Heartseed glow candidates: with `tools/content-search`, list weapon-glow enchants that fit each path (Xuen: lightning or white tiger; Chi-Ji: red or gold fire; Niuzao: earth or stone; Yu'lon: jade or green water) per stage, with enchant ids and models, in `docs/agents/heartseed-glows.md`. Data only; Claude picks | `docs/agents/heartseed-glows.md` | 3 to 6 candidates per path, each id found by the tool | flash | open |

## Suggested split

1. **Claude Code:** T-001 first (it blocks everything). Then the Heartseed track (T-020 to T-026),
   picking up T-030 and T-031 when the jewel needs visuals and tuning, and T-048 (GM test suite) before
   the first human playtest.
2. **Codex**, once T-001 is done: fast iteration (T-005 to T-009, T-035, T-037 to T-040) and the cheap
   tools (T-032 to T-034), then the quick wins, starting with T-045 (priority) and T-010 to T-013.
3. **Gemini:** tooling first, no full server builds while it settles in: T-034 (SQL validator), then
   T-030 (DBC reader and content search), then T-050 and T-033. When Argon's weekly quota runs out, Flash
   works through the Flash queue. Claude Code reviews its PRs.
4. **Human:** T-036 (timeline), the hardware target for T-003, and PR reviews.
