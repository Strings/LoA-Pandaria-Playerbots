# T-038 (part): Ninja + sccache build script

**Agent:** claude
**Branch:** claude/T-038-ninja-sccache
**Task status:** in progress (this PR is one piece of T-038; hot reload, RNG seed and lld-link are still open)

## What changed
- `tools/build/build.ps1` and README: configure and build any checkout with Ninja and sccache (the
  human approved installing both: `scoop install sccache ninja`). Loads the MSVC environment itself,
  sets Boost 1.85 over the machine's 1.81 `BOOST_ROOT`, passes the OpenSSL libraries, copies the runtime
  DLLs (including OpenSSL's `legacy.dll`, Codex's T-032 find) next to the binaries.
- One shared sccache cache for all checkouts (`SCCACHE_BASEDIRS` = main checkout and both worktrees).
- Root `CMakeLists.txt`: quote `CMAKE_COMMAND` in the `-E time` compile/link launchers; the unquoted
  `C:/Program Files/...` broke every Ninja link rule.
- `modules/CMakeLists.txt`: the playerbots `modules` library keeps `/Zi` and compiles without sccache. With
  embedded debug info (`/Z7`) it passes the 4 GB `.lib` limit (LNK1248).
- `BigNumber.cpp`: include `Errors.h` (it relied on the precompiled header for `ASSERT`).
- `.gitignore`: `build/` matched `tools/build/`; added an exception.

## How it was tested
- Full Ninja build of the Claude worktree (with PCH): about 50 min, same as the Visual Studio build.
- One-file change in a script plus relink: **1.1 min**.
- sccache on the `shared` library: second build from scratch 74 to 92 % cache hits without PCH.

## Decisions made
- PCH stays on by default. sccache cannot cache PCH compiles (`/Yc`, `/Fp`: 878 non-cacheable calls in a
  full build), so caching only pays off once the code builds without PCH. `-NoPch` exists for that.

## For the other agent
- Use `tools\build\build.ps1` from your worktree once this merges. Binaries go to `build\bin\` (no config
  subfolder). The `.conf.dist` files are not copied by the Ninja build; copy them from `src/`.
- First build per checkout is still a full build; with PCH on the cache helps little between worktrees yet.

## Next
- Make the code build without PCH (`-NoPch`, then `ninja -k 0` to list every missing include), then turn
  PCH off by default so the cache covers game and scripts.
- Split the playerbots module into smaller libraries so it can use `/Z7` and the cache.
- lld-link, hot reload, RNG seed: the rest of T-038.

## Map additions
- `tools/build/`: build script and notes.
