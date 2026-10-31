# Handoff 001: from design to first build

**Date:** 2026-10-31. **From:** the design sessions in Claude (claude.ai). **To:** Claude Code, first session.

The design is done for now. This handoff gets the project from a folder of documents to a running,
benchmarked server that agents can build on. Read this file, then `AGENTS.md`, `STATUS.md` and
`TASKS.md`. Read `docs/DESIGN.md` only in the sections your task touches.

## Where things stand

- `I:\UltimateMoP` is **not a git repo yet**. It holds the design doc, the agent coordination files
  and the game client. Nothing has been forked, cloned or built.
- `docs/DESIGN.md` is the full design (exported 2026-10-31 from the living design doc). The roadmap,
  milestones and decided rules are near the end under "Roadmap and task list".
- `AGENTS.md` holds the rules for Claude Code and Codex, who work in parallel in separate worktrees.
- Your first task is **T-001** in `TASKS.md`. It blocks everything else, so do it before anything on
  the jewel track.

## The game client: `mop-clean/`

`I:\UltimateMoP\mop-clean\` is a complete **5.4.8.18414 enUS** client (about 24 GB). Facts:

- `Data/` holds the base MPQs and update MPQs up to 18273, plus `Data/enUS/`. This is the source for
  extracting `dbc`, `maps`, `vmaps`, `mmaps` and `cameras`.
- Four executables. `Wow.exe` and `Wow-64.exe` are the stock 32- and 64-bit clients. `_Wow.exe` and
  `_Wow-64.exe` skip file verification, which lets custom patch MPQs load; use those for anything
  that needs our client patch later. (The Hungarian `readme.txt` says the same; the client came from
  Tauri.)
- `WTF/Config.wtf` still points `realmlist` at `hu.logon.tauri.hu`. Change it to the local auth
  server only when someone actually launches the client to play or test.
- To connect to our server, the exe needs the 5.4.8 connection patch from
  `contrib/SkyFire Patcher/548` (in the LoA repo). Patch a **copy** of the exe, never the originals.

Rules for this folder:

- **Never commit it.** Add `mop-clean/` to `.gitignore` (and `.git/info/exclude`) before the first
  `git add`. It is 24 GB of Blizzard data.
- **Keep it clean.** Treat it as read-only input. Some extractors write their output into the current
  directory; run them with input and output path options, or from a separate output folder, so
  nothing lands inside `mop-clean/`. If something does land there, list it and ask before removing it.
- Launching the client is computer use: ask first (AGENTS.md). Use the headless client for tests.

## T-001: fork, build, extract, import

1. **Fork and clone into this folder.** Fork https://github.com/1LineAtaTime/LoA-Pandaria-Playerbots
   to the human's GitHub account (`gh repo fork` if `gh` is installed and signed in; otherwise ask the
   human to fork it in the browser and give you the URL). Then follow `SETUP.md` step 1 to turn
   `I:\UltimateMoP` into the repo on the `playerbot` branch, with `origin` as the fork and `upstream` as
   LoA. Add `.worktrees/`, `mop-clean/` and `gamedata/` to `.gitignore` first.
2. **Prerequisites.** Read the LoA README for the Windows toolchain (Visual Studio, CMake, MySQL,
   Boost, OpenSSL and versions). Check what is installed. Installing or changing system toolchains is
   outside the repo: list what is missing and ask the human before installing.
3. **Build.** Configure with `TOOLS=ON` so the extractors build too. Build into `build/`.
4. **Extract game data** from `mop-clean/` into `I:\UltimateMoP\gamedata\` (gitignored, shared,
   read-only once done): `dbc`, `maps`, `cameras`, then `vmaps`, then `mmaps`. mmaps generation takes
   hours; run it in the background and keep going.
5. **Databases.** Create `auth`, `characters`, `world` and `playerbots` from the release dumps and
   SQL (the playerbots DB is imported by hand; the SQL updater doesn't cover it yet). Then the per-agent
   sets from AGENTS.md (`*_claude`, `*_codex`), by hand until T-002 scripts it.
6. **Configure and run.** `worldserver.conf` and `playerbots.conf` from the `.dist` files, `DataDir`
   pointing at `gamedata/`, SOAP on. Start auth and world servers, confirm they come up, and create a
   GM account.
7. **Worktrees.** `SETUP.md` step 2, so Codex can start on its tasks.
8. **Report.** Status fragment `status/claude-T-001.md`: what was installed, build times, extraction
   times, anything that broke, and paths worth adding to `docs/MAP.md`. Commit the coordination files
   and push to the fork (SETUP.md step 1 lists them).

Pushing to `playerbot` is an ask-first action (AGENTS.md). Confirm with the human before this one
setup push of the coordination files.

## After T-001

- **Tell the human** the server is up, and ask for the decisions only they can make (below).
- **Codex** can start on fast iteration, tools and quick wins (suggested split in `TASKS.md`).
- **You** move to the Heartseed track (T-020 onward). It is the top priority.

## Decisions waiting on the human

- **Server timeline** (T-036): what time it is in the world (the intro is at Theramore in 5.0,
  leveling runs through the war, the Vale is post-Siege). The Heartseed intro and story flags depend on it.
- **Hardware target:** the bot baselines (T-003) only mean something on the machine the human plays on.
- **Starting Heartseed numbers:** placeholders are fine; pacing is tuned in v0.5 once every Growth
  source exists ("Accepted interim states" in DESIGN.md).

## Rules from the design that matter most early

All of these are in `AGENTS.md` or `docs/DESIGN.md`; they are repeated here because they shape every
early task.

- **Build for the end state.** Build systems in their final shape even if incomplete; don't build a
  stopgap the long-term plan would break unless replacing it is an easy fix.
- **Every timer is a config key; core changes go behind config keys.** Keeps the fork easy to rebase.
- **Scripted first.** Anything that will later use an LLM ships a scripted provider first, behind an
  interface. Tests always run on the scripted provider.
- **Turn telemetry on early** (T-004). Every play session before it exists is lost training data.
