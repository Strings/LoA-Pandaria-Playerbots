# T-002 + T-005: Per-agent setup and test/load profiles

**Agent:** codex
**Branch:** codex/T-002-T-005-devenv
**Task status:** review

## What changed
- tools/devenv/setup.py creates agent-specific auth/world/playerbots configs, correct DB suffixes and ports, optional missing-database import, realmlist update and GM creation, and explicit-config hidden Windows launchers.
- Copies existing binaries and all four runtime DLLs, including legacy.dll. No builds or toolchain changes.
- profiles.json: dev/test timers and reduced logging; none/local/scale bot loads. Local requires 20..50 named random bots and a configurable zone anchor; stage_local.py logs in/stages them. Scale requires --performance.
- Transport grid loading off; no general grid-preload config exists in this fork. Optional profile.d JSON overlay interface supports independent system PRs.

## How it was tested
- Three unit tests: override/duplicate handling, roster bounds/same anchor, all profile keys verified in the fork.
- Imported four newly created private fixture DBs; second initialization skipped them and preserved rows. Fixture DBs removed afterwards.
- Generated Codex none/test runtime from existing T-001 binaries, copied legacy.dll and set realmlist to 8105; server initialized in 31 seconds, reported bots disabled, headless login passed on 3744/8105, and SOAP shutdown returned success.
- Local roster/staging generation tested; no 20-bot or scale performance run claimed (fixtures are T-007).

## For the other agent
- Use setup.py --agent claude from your worktree when ready. Existing populated DBs are skipped. GM password is never reset.
- Legacy provider must be copied beside the exe; main checkout already contains it per the human.
- Overlay interface: profile.d/*.json contains dev/test maps with world/bots maps; {repo} expands to the worktree. T-009 can add Eluna independently.
- No full build. No main/Claude DB changes or global settings.
