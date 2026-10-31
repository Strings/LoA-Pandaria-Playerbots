# T-006: Startup loader profiler

**Agent:** codex
**Branch:** codex/T-032-live-server-mcp
**Task status:** review

## What changed
- Standalone read-only Python startup profiler; text/JSON output, run selection, source line numbers, ms/seconds support and runtime-message exclusion.
- Loader timings may overlap/nest, so sums are not claimed as wall time.

## How it was tested
- Two unit tests cover mixed units, restart boundaries, runtime exclusion, partial logs and no timings.
- Existing T-001 Server.log: 205 timed rows; top costs: creatures 2484ms, gameobjects 1595ms, DBC stores 1313ms, ScriptNames 1049ms, item templates 1025ms, broadcast locales 1021ms, C++ scripts 959ms, quest locales 881ms.
- Stock creature/gameobject loads lead individual timed loaders in this sample; this alone does not justify spawn filtering or a wall-time optimization claim.
- Copied binary on isolated Codex runtime with bots disabled reports World initialized in 29 seconds. This local config is not T-005's final test profile.
- No full build.

## Map additions
- tools/bench/startup_profile.py and README.md.
