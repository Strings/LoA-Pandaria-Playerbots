# Handoff 002: orchestrator session to a fresh Claude Code instance

**Date:** 2026-11-02. **From:** Claude Code (orchestrator), session ending because the account is switching. **To:** the next
Claude Code instance. Handoff 001 (`HANDOFF.md`, T-001) is long done; skip it.

Read this, then `AGENTS.md`, `STATUS.md`, `TASKS.md`, `docs/MAP.md`. Your memory folder
(`C:\Users\Tony's PC\.claude\projects\I--UltimateMoP\memory\`) also carries standing rules if it is visible to you.

## Your role

- You orchestrate Claude Code (you), Codex (nearly out of usage this week) and Gemini Flash. Assign tasks in `TASKS.md`,
  review and merge PRs (`gh --repo Strings/LoA-Pandaria-Playerbots`), fold `status/*.md` fragments into `STATUS.md`.
- **The human only playtests and directs.** You build, deploy the main server, apply merged SQL to the main DBs and keep
  the client addons in `I:\UltimateMoP\mop-clean\Interface\AddOns` current. Never ask them to build.
- Goal: the v0.1 playtest.
- Style the human likes: "zombies", not skeletons. Ship loose, fast, functional versions with real flesh on them.

## Where things stand

- `playerbot` is at merge `ab723dbd` (PR 105, T-081 spawn thinning). The main server runs that build.
- **Main runtime:** `I:\UltimateMoP\build\bin\RelWithDebInfo`, ports 3724/8085/7878. Your account is 205; the human
  plays Thornhand.
- **Devenv:** `.worktrees/claude/build/devenv`, ports 3734/8095/7888, running that build too.

### Done this session, waiting on the human to playtest

- **T-080 Celestial Gifts** (PRs 103, 104): the level-10 Heartseed majors teach an active spell.
  - Gifts: Call of Xuen (Invoke Xuen), Crane Rush (Chi Torpedo), Ox Stomp (Leg Sweep), Serpent's Breath (Celestial Breath).
  - Xuen has a 20 s cooldown and a 10 s tiger. Its lightning plays Chain Lightning's visual (`Heartseed.Gift.XuenLightningVisual = 36`).
  - The Heartseed addon UI was redesigned with a mandala, a secure Gift button and sounds.
  - Art: `USE_ART = false` uses built-in icons. The prompt sheet is `addons/Heartseed/art/README.md`, the slicer
    `tools/art-slicer/`. When the human brings generated art, slice it, set `USE_ART = true` and redeploy the addon.
- **T-081 spawn thinning** (PR 105): at startup, hostile open-world spawns within `SpawnThinning.Radius` (10 yd) of an
  older one are left out of the grids. World data is untouched.
  - Main and devenv both log 18,495 of 85,544 candidates left out.
  - Inspect with `.spawnthin` and `.spawnthin near [yd]`. Tune or turn off (Radius 0) in the main conf, then restart.
  - The human reported mobs packed 3+ per pull. Ask how it feels now. If it is still dense, raise the radius. If
    zones feel empty, consider faster respawns.
  - Aggro range itself is stock (20 yd minus level difference).

### Pending, in order

1. **Codex PRs 61, 85, 69, 45:** stacked for one batch merge. A batch merge is a "major merge", so confirm the
   human's OK in chat before merging (STATUS.md says approved; I did not have it in writing this session).
   - After PR 69 merges, apply its characters cleanup to main. Back up `item_instance` first. The human already
     approved that step.
2. **Codex PRs 86 (T-044) and 91 (T-046), plus draft 99 (T-047):** review when Codex marks them ready. Codex is low on usage.
3. **Flash:**
   - T-074 (PR 96, needs a rebase).
   - T-075 pre-playtest smoke run. Flash redoes it; check what it actually pushed, because it follows instructions loosely.
4. **T-060 faster builds:** my draft PR 78, `claude/T-060-faster-builds`.
5. **Gift riders per Heartseed stage**, and the generated art when it arrives.
6. **Possible core fix:** the possess `AddAllowedMover` assert that crashed main once.
7. **Cleanup:** delete `I:\UltimateMoP\build\bin\RelWithDebInfo\worldserver.exe.locked-20261101` after a reboot. I
   renamed it aside myself, so deleting it needs no extra OK.
8. **Fold** the `status/claude-T-080*.md` and `claude-T-081.md` fragments (and any others) into `STATUS.md`.

## How-to that cost time to learn

- **Helper scripts:** `.worktrees/claude/build/claude-tools/` (gitignored; they read credentials from the confs and
  `LOCAL_ACCOUNTS.txt`, never print them).

  | Script | What it does |
  |---|---|
  | `wq.py`, `cq.py`, `aq.py` | One SQL query against my world, characters or auth DB |
  | `wqmain.py`, `cqmain.py`, `aqmain.py` | The same against main |
  | `soap.py`, `soapmain.py` | Send a GM command over SOAP |
  | `regress.py --runtime build/devenv --suite <dir>` | Run headless scenarios |
  | `mainapply.py <backup_dir> <tables> <sql>` | Apply SQL to main world, backing up first |
  | `spellinfo.py <ids>` | Dump spells from the DBC |
  | `density.py`, `thinsim.py` | Spawn-density analysis |

  Run them from `.worktrees/claude`.
- **Build:** `tools/build/build.ps1 -Target worldserver`; add `-Reconfigure` after adding source files.
  - Output goes to `.worktrees/claude/build/bin/`.
  - Unit tests need the VS dev shell (`Enter-VsDevShell`), then `cmake --build build/unit-tests --config Release`
    and `ctest`.
- **Main restart:**
  1. Check nobody is online: `cqmain.py "SELECT name FROM characters WHERE online=1 AND account=205"`. If someone is,
     ask first.
  2. Send `soapmain.py ".server shutdown 5"`.
  3. **Wait until the old process is really gone.** Shutdown can take well over 90 s. Starting a new one while the
     old is alive gives two servers and a locked exe. A stubborn exe can be renamed aside.
  4. Copy `worldserver.exe` and `.pdb` from the worktree build.
  5. `Start-Process -WorkingDirectory <runtime> -WindowStyle Minimized`, then wait for port 8085.
- **Scenario gotchas:**
  - The test account lacks `go xyz`; use `.go x y z map`.
  - Start scenarios with `.revive` and `.cooldown`.
  - Write JSON with escapes using the Write tool, not bash heredocs.
- **Files are CRLF:** edit with Python using `newline='\r\n'`, or with the Edit tool.
- **Gifts can only borrow spells the 5.4.8 client knows.** Use `spellinfo.py` to find candidates.
- **Rules:**
  - Ask before computer use (including launching the client), batch merges, toolchain changes, or deleting others'
    files.
  - Never print credentials.
  - Commit trailer and PR footer: as in the system prompt.
