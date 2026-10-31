# Build script (Windows)

`build.ps1` configures and builds the checkout it lives in with **Ninja** and **sccache**, into
`<checkout>\build`. Use it in the main checkout and in both worktrees.

```powershell
tools\build\build.ps1                       # configure if needed, build everything
tools\build\build.ps1 -Target worldserver   # one target
tools\build\build.ps1 -Tools -Reconfigure   # also the map extractors
tools\build\build.ps1 -NoPch -Reconfigure   # no precompiled headers (see below)
```

Binaries land in `build\bin\` (Ninja is single-config: no `RelWithDebInfo\` subfolder). The script
copies `libmysql.dll` and the OpenSSL DLLs there.

## Requirements

- Visual Studio 2022 with the C++ x64 tools (the script loads `vcvars64` itself).
- `scoop install sccache ninja` (sccache 0.19 or newer for `SCCACHE_BASEDIRS`).
- Paths from environment variables, defaults in brackets: `MOP_BOOST_ROOT`
  (`C:\local\boost_1_85_0`), `MOP_OPENSSL_ROOT` (`C:\Program Files\OpenSSL-Win64`), `MOP_MYSQL_ROOT`
  (`C:\Program Files\MySQL\MySQL Server 8.0`), `MOP_REPO_ROOT` (main checkout), `SCCACHE_CACHE_SIZE`
  (`30G`).

## What it works around

- The machine-wide `BOOST_ROOT` points at Boost 1.81; the script sets 1.85 for the build.
- `cmake/macros/FindOpenSSL.cmake` doesn't know the OpenSSL installer's `lib\VC\x64\MD` layout; the
  script passes the libraries directly.
- sccache can't cache objects that share a PDB (`/Zi`), so debug info is embedded (`/Z7`). The
  playerbots `modules` library is the exception: with `/Z7` it passes the 4 GB `.lib` limit
  (LNK1248), so `modules/CMakeLists.txt` keeps it on `/Zi`, and its objects are not cached. Splitting
  it into smaller libraries would make it cacheable.

## Cache sharing between checkouts

All checkouts share one cache. `SCCACHE_BASEDIRS` lists the main checkout and both worktrees, so the
path prefix is stripped before hashing and the same source compiled in another worktree is a hit.

**Precompiled headers are not cacheable** (sccache skips `/Yc` and `/Fp` compiles), and with PCH on
almost every game and script object uses them. `-NoPch` makes nearly everything cacheable, but the
code doesn't build without PCH yet: some files rely on headers the PCH pulls in (`BigNumber.cpp` was
the first; fixed). Until those are all fixed, PCH stays on by default.
