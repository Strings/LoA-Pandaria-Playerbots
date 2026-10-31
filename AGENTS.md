# AGENTS.md: rules for coding agents on UltimateMoP

This repo is a fork of LoA-Pandaria-Playerbots (MoP 5.4.8, build 18414) with the mod-playerbots
port. We are turning it into a solo-first MoP server (up to 8 friends) with living bots, an
artifact (the Heartseed), war fronts and more. Claude Code, Codex and Gemini (Antigravity) work here,
usually at the same time, on separate tasks. Claude Code also orchestrates: it assigns tasks in
`TASKS.md` and reviews the other agents' PRs. These rules let you work autonomously without getting in
each other's way.

## Start of every session

1. Read `STATUS.md` (where the project is), `TASKS.md` (your task) and `docs/MAP.md` (where
   things live). Do not read the git log unless your task needs history. If a `HANDOFF.md` is
   addressed to you and not yet done, start there.
2. Read only the sections of `docs/DESIGN.md` your task touches. It is long; it is the design
   reference, not a checklist.
3. Work in your own worktree on your own branch (see Worktrees). Pull the latest
   `playerbot` branch into it before starting a new task.

## Autonomy

You are expected to finish tasks mostly on your own. Inside your task's scope, act; don't ask.

**Do without asking**

- Read anything in the repo.
- Create and edit files in the paths your task owns, plus small incidental edits elsewhere that
  the task obviously needs (registering a script in `ScriptLoader.cpp` or `CMakeLists.txt`,
  adding a key to a `.conf.dist`, a one-line fix to a call site).
- Build, run tests, start and stop the auth and world servers on your own ports, use your own
  databases, run `.botscenario` and the headless client.
- Install packages from the standard registries when a task needs them for tooling or tests.
- Commit on your branch, rebase on `playerbot`, push your branch, open a PR, and merge a routine PR of
  your own once it is rebased, builds cleanly and passes its tests.
- Implement items marked provisional in `docs/DESIGN.md` when your task asks for them, exactly as
  described. Note in your status fragment that they were provisional.

**Stop and ask first**

- **Scope breaches:** substantial changes outside your task's owned paths, or work that turns out
  to be much bigger than the task describes.
- **Judgment calls the design doesn't settle:** a design choice not covered by `docs/DESIGN.md`,
  anything that contradicts a decided rule, or a requirement ambiguous enough that a wrong guess
  is costly to undo.
- **Destructive or shared actions:** deleting files or directories you didn't create, dropping or
  rewriting tables in existing databases, schema changes to existing core tables, force-pushing,
  rewriting history, pushing to `playerbot` other than by merging a PR, major merges (a batch of many
  PRs, one that is risky to undo, or one that conflicts with another agent's work), touching another
  agent's worktree, databases or ports.
- **Dependencies:** adding a third-party library to the build or changing toolchain versions.
- **Computer use:** anything that drives the GUI: launching or controlling the game client,
  browser automation, desktop apps. Use the headless client for tests instead.
- **Outside the repo:** changing system settings, files outside `I:\UltimateMoP`, or anything
  that costs money (renting servers or GPUs).

**How to ask:** stop the risky step, mark the task `blocked` in `TASKS.md` with the question,
write a status fragment that starts with `BLOCKED:`, and ask in your session. Keep doing any part
of the task that doesn't depend on the answer.

## Worktrees, branches, commits

- Worktrees live in `.worktrees/<agent>/` (gitignored). Claude Code uses `.worktrees/claude`,
  Codex uses `.worktrees/codex`, Gemini uses `.worktrees/gemini`. Never edit files in the main checkout
  or another agent's worktree.
- Branch names: `<agent>/<task-id>-<slug>`, e.g. `claude/T-020-jewel-table`.
- Small, focused commits with imperative messages: `Jewel: add character_jewel table`. Prefix with
  the area (`Jewel:`, `Bots:`, `Travel:`, `Tools:`, `Docs:`).
- Before opening a PR: rebase on `playerbot`, build cleanly, run the relevant tests, add your
  status fragment, and update your task's line in `TASKS.md`.
- Another agent reviews your PR when asked; Claude Code reviews Gemini's PRs before they merge while
  Gemini is new here. You merge routine PRs yourself; ask the human before a major merge (see Stop and
  ask first). Never edit another agent's branch to make a merge go through.

## Shared hotspots

These files are edited by everyone. Keep edits minimal and append-only where possible:

- `src/server/scripts/ScriptLoader/ScriptLoader.cpp`, the `CMakeLists.txt` files,
  `src/server/worldserver/worldserver.conf.dist`, `modules/mod_playerbots/config/playerbots.conf.dist`.
- `STATUS.md` is never edited by agents directly. Write a fragment instead (see Status).

## SQL updates

- World and characters updates go in `sql/updates/world/` and `sql/updates/characters/`;
  playerbots updates in `modules/mod_playerbots/data/sql/playerbots/updates/db_playerbots/`.
- File names: `YYYY_MM_DD_<agent>_<NN>_<description>.sql`, e.g.
  `2026_11_02_claude_00_jewel_table.sql`. `NN` counts up per agent per day. The agent name in the
  file name keeps the two agents from colliding.
- Updates must be idempotent (`DELETE`-then-`INSERT`, `CREATE TABLE IF NOT EXISTS`, scoped
  `UPDATE`s), matching the existing files.
- `Updates.EnableDatabases = 0` is the shipped default, so apply your updates to your own
  databases by hand and say so in the fragment.

## Your test environment

Each agent has its own databases and ports so tests never collide. Set the matching `MOP_*`
environment variables for the headless client (`contrib/headless-client/README.md`).

| Who | Auth port | World port | SOAP port | Databases |
|---|---|---|---|---|
| Human (main checkout) | 3724 | 8085 | 7878 | `auth`, `characters`, `world`, `playerbots` |
| Claude Code | 3734 | 8095 | 7888 | `auth_claude`, `characters_claude`, `world_claude`, `playerbots_claude` |
| Codex | 3744 | 8105 | 7898 | `auth_codex`, `characters_codex`, `world_codex`, `playerbots_codex` |
| Gemini | 3754 | 8115 | 7908 | `auth_gemini`, `characters_gemini`, `world_gemini`, `playerbots_gemini` |

- Build into `build/` inside your worktree. Use the shared compiler cache if it is set up
  (`sccache` on Windows, `ccache` on Linux).
- Game data (`dbc`, `maps`, `vmaps`, `mmaps`, `cameras`) lives in `I:\UltimateMoP\gamedata\`
  (gitignored), shared and read-only. Point `DataDir` at it; never regenerate it unless your task
  says so.
- The game client is `I:\UltimateMoP\mop-clean\` (5.4.8.18414 enUS, gitignored). Treat it as
  read-only input: never commit it, never write into it (extract elsewhere, patch copies of the exe).
  Launching it is computer use, so ask first.
- Your `realmlist` row in your own auth database must point at your world port.

## Testing and iteration

- **Prototype content in Lua first.** Eluna is compiled into the core (`Eluna.Enabled`, scripts in
  `lua_scripts/`) and reloads while the server runs. Rares, events, gossip and dialogue start as Lua
  prototypes; port to C++ when they need speed or deeper core access.
- **Every timer is a config key.** War front cycles, respawns, resets, cooldowns, heat meters: never
  hardcode a duration. The test profile shortens them.
- **Use the test profile** (`tools/devenv/`) and the smallest bot load profile that covers your task:
  none, local (20 to 50 bots in one zone) or scale (performance work only).
- **Pure logic gets unit tests.** Keep math and rules (scaling, averaging, rolls, schedulers) in
  server-independent code and test it with the unit test framework.
- **Run the regression suite** (headless-client scenarios) against your test profile before a PR.
- **Use fixtures and state commands** instead of playing to a state: fixture characters, database
  snapshots, GM commands for jewel rank, corruption and similar.

## Design rules for every task

These come from decided rules in `docs/DESIGN.md` and apply everywhere.

- **Build for the end state.** Build a system in its final shape, even if parts stay incomplete
  until later phases. Don't build a stopgap the long-term plan would break, unless replacing it later
  is an easy fix (a config value, one lookup). "Accepted interim states" in DESIGN.md lists the
  allowed gaps; anything else, ask.
- **Shared modules are built once,** at their first use, with the interface the later users need:
  the affix module (rares, corruption dial, Brawler mutators, wild gems, consumable modifiers), the
  call board (battlegrounds, ambushes, war fronts, dungeons), the storyteller director, the rumor
  interface, detached item storage (gear pages, reagent bank, twink templates).
- **Scripted first, LLM later.** Anything that will use an LLM ships a scripted provider first, behind
  a narrow interface. Decisions read structured data only, never free text. Tests and the test
  profile always run the scripted provider.
- **Hot-reloadable data.** Custom tables (traits, techniques, plays, behaviors, echoes, affixes,
  director parameters) get a `.reload` command once T-038 lands.
- **GM hooks for every system.** Each system adds GM commands to force and inspect its state
  (spawn it, trigger it, set it, explain it), so it can be tested without playing to it.
- **Sealed secrets.** When you implement a hidden secret from a brief, keep the solution out of PR
  descriptions, status fragments and commit messages. Record it only in the sealed secrets log (path set
  by the secrets task) and give it a hint ladder.
- **Cross-expansion content** appears in every expansion's zones where a local race or faction theme
  fits; skip it where none fits.

## Definition of done

- Builds with no new warnings in touched files.
- Tested: unit tests for new pure logic, plus a regression-suite scenario, `.botscenario` or GM command
  check for in-game behavior, with the steps in the PR.
- Any new duration is a config key.
- New config keys documented in the relevant `.conf.dist` with a sensible default.
- New player-facing systems ship with a brief introduction: an intro quest offered at the level the
  system unlocks, or a short popup if a quest doesn't fit. Once their frameworks exist (Phase 1b),
  they also ship with their achievements, journal entries and chronicle events.
- New custom tables ship as idempotent SQL updates that have been run against a copy of a real save
  (T-037), so a later change can never break an existing character.
- GM commands to force and inspect the new system exist.
- Status fragment written; task line updated in `TASKS.md`.

## Status fragments

Every PR adds `status/<agent>-<task-id>.md` from `status/_TEMPLATE.md`: what changed, what's next,
anything the other agents must know, and any decision you made. The human folds fragments into
`STATUS.md` after merging and deletes them. Never edit `STATUS.md` yourself.

## Build it modular

Design principle: build systems as reusable modules with clean interfaces, so they can serve other
projects too. In practice:

- New systems get their own folder (e.g. `src/server/scripts/Custom/Jewel/`, `tools/<tool>/`) with a
  short README covering purpose, interface and config.
- Keep game-agnostic logic separate from WoW-specific glue where it's cheap to do so (the
  consciousness system, placement tool, classifiers and MCP servers each get a core plus an adapter).
- Core-game changes go behind config keys and stay as small as possible, so the fork stays easy to
  rebase on upstream.
- Tools take paths, ports and credentials from config or environment variables, never hardcoded.

## Code style

Follow the surrounding code (TrinityCore style: 4-space indent, braces on new lines, `CamelCase`
types and methods, `m_` or `_` member prefixes as nearby code does). Use the existing logging
channels; playerbots features get their own log channel. Comments explain why, not what.
