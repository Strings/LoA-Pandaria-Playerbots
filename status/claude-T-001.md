# T-001: Fork, build, extract, import, run

**Agent:** claude
**Branch:** claude/T-001-setup
**Task status:** review

## What changed
- Forked LoA to `Strings/LoA-Pandaria-Playerbots`. `I:\UltimateMoP` is the repo on `playerbot`
  (`origin` = fork, `upstream` = LoA). `mop-clean/`, `gamedata/`, `.worktrees/` are in `.gitignore`
  and `.git/info/exclude`. `docs/claude-settings.json` moved to `.claude/settings.json`.
- Coordination files committed on `playerbot` locally (e68d7112, not pushed: pushing `playerbot` is
  ask-first). This PR carries that commit too, so merging it is the setup push.
- Worktrees: `.worktrees/claude` (`claude/base`) and `.worktrees/codex` (`codex/base`).
- Built the main checkout into `build/` (RelWithDebInfo, `TOOLS=ON`).
- Extracted `dbc`, `maps`, `cameras`, `vmaps`, `mmaps` into `I:\UltimateMoP\gamedata\` (6.6 GB).
  `mop-clean/` checked byte-for-byte unchanged afterwards (file list, sizes, mtimes).
- Imported all twelve databases from the **v1.0.4** release dump (`playerbot` is v1.0.4 plus 9
  commits with no SQL changes; the v1.1.0-beta and CI dumps are for `playerbot-beta` and are ahead).
- `auth_claude.realmlist` points at 8095 ("MoP (Claude)"), `auth_codex.realmlist` at 8105
  ("MoP (Codex)").
- Configs written next to the binaries in `build/bin/RelWithDebInfo/` (`authserver.conf`,
  `worldserver.conf`, `playerbots.conf`): MySQL user `ultimatemop`, `DataDir` =
  `I:/UltimateMoP/gamedata`, SOAP on 7878 (shipped default). Bot population left at the shipped
  500.
- GM accounts: `GM` in `auth` and `CLAUDE` in `auth_claude`, gmlevel 3 on all realms. Passwords are
  in the gitignored `build/bin/RelWithDebInfo/LOCAL_ACCOUNTS.txt`.

## Installed / environment
- Nothing installed. Already present: VS 2022 Community (MSVC 14.39), Windows SDK 10.0.22621,
  CMake 3.28.3, MySQL 8.0.36 (service `MySQL80`), Boost 1.85 (`C:\local\boost_1_85_0`), OpenSSL in
  `C:\Program Files\OpenSSL-Win64`, Python 3.9, `gh` (signed in as Strings).
- New MySQL user `ultimatemop`@`localhost` (password `ultimatemop`, dev only) with ALL on
  `auth%`, `characters%`, `world%`, `playerbots%`. The human created it as root.

## Timings (6 cores)
- CMake configure: about 9 s. Full build: **46 min** (no compiler cache; sccache is not installed).
- Extraction: maps/dbc/cameras 73 s, vmap extract 708 s, vmap assemble 272 s, **mmaps 71 min**.
- Database import per set: world about 11 to 15 min, the rest under a minute each.
- Worldserver start: 4 min 46 s on the first start (creates 200 bot accounts and 2200 bot
  characters), 22 to 50 s after that.

## How it was tested
- authserver lists the realm; worldserver reaches "World initialized" with mmaps and vmaps loaded.
- SOAP `server info` as `GM` returned stats (bots logging in, 180 connections at 30 s uptime).
- Clean shutdown via SOAP `server shutdown 1` (21 s).
- Not tested: a real or headless client login (no client launch without asking).

## Gotchas (for the other agent and T-002)
- **Boost:** the machine has system and user `BOOST_ROOT=C:\local\boost_1_81_0`, and
  `dep/boost/CMakeLists.txt` takes `BOOST_ROOT` from the environment. Set
  `$env:BOOST_ROOT='C:\local\boost_1_85_0'` in the shell before configuring or building.
- **OpenSSL:** the repo's `cmake/macros/FindOpenSSL.cmake` doesn't know the installer's
  `lib/VC/x64/MD` layout. Pass the libraries explicitly:
  `-DLIB_EAY_RELEASE=".../lib/VC/x64/MD/libcrypto.lib" -DLIB_EAY_DEBUG=".../lib/VC/x64/MDd/libcrypto.lib"
  -DSSL_EAY_RELEASE=".../lib/VC/x64/MD/libssl.lib" -DSSL_EAY_DEBUG=".../lib/VC/x64/MDd/libssl.lib"`.
  The install is mixed: headers say 3.0.13, DLLs are 3.2.1 (ABI-compatible within 3.x; ran fine).
- **Runtime DLLs:** copy `libmysql.dll` (MySQL `lib/`) and `libcrypto-3-x64.dll`, `libssl-3-x64.dll`
  (OpenSSL `bin/`) next to the binaries.
- **`playerbots.conf` is read from the current working directory**, not the exe folder. Run each
  worldserver from its own folder.
- **The Windows console ignores piped stdin** (`ReadConsoleW`), and a worldserver with a pipe for
  stdin spins a core in the CLI thread. Start it with its own console (`Start-Process -WindowStyle
  Hidden`) and drive it over SOAP. To make the first GM account without a console, insert it into
  `account` with `sha_pass_hash = UPPER(SHA1(UPPER('USER:PASS')))` plus an `account_access` row.
- `DBErrors.log` has about 620 lines on a clean start (stock data warnings); not ours.

## Decisions made
- Database source: v1.0.4 dump (see above).
- MySQL user `ultimatemop` for every set instead of root (the human chose this).

## Next
- The human: review and merge this PR (it includes the coordination commit for `playerbot`).
- The human: copy `docs/codex-config.example.toml` into `%USERPROFILE%\.codex\config.toml`.
- Codex can start (TASKS.md "Suggested split"). The worktree builds need their own `build/`; with no
  compiler cache each one costs about 46 minutes. Installing sccache is worth asking about.

## Map additions
- `build/bin/RelWithDebInfo/`: the main checkout's binaries, confs, `Logs/`, `LOCAL_ACCOUNTS.txt`.
- `gamedata/`: extracted data plus `extract_*.log` from each step.
- `src/server/worldserver/CommandLine/CliRunnable.cpp`: the console thread (stdin handling above).
- `src/server/game/Accounts/AccountMgr.cpp`: account creation and the password hash.
- `modules/mod_playerbots/src/mod_playerbots.cpp`: where `playerbots.conf` is loaded.
