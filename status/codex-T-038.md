# T-038: Custom-table reload and core test RNG

**Agent:** codex
**Branch:** codex/T-038-reload-rng
**Task status:** review (draft; full server/GM checks deferred)

## What changed
- Custom/Reload owns a server-independent callback registry and CommandScript/WorldScript adapter. Registered tables support reload <table>, canonical reload custom <table>, and reload custom list. Built-in reload commands keep precedence; existing command-tree merging supplies the fallback without modifying ScriptMgr.h or cs_reload.cpp.
- Registry rejects invalid/duplicate names, overlapping/reentrant reloads and catches loader errors. RegisterGroup shares one loader/guard across related table aliases, with all-or-nothing name validation. Owning modules stage/validate snapshots and atomically publish on success; README documents cache lifetime/readers and registration API. The Heartseed adapter is prepared in an ignored integration copy, pending the owning Heartseed PRs landing during Claude's merge pass.
- Random.h common APIs SetRandomSeed/ClearRandomSeed/GetRandomSeedState and GM rng seed/off/status. Enabled test mode serializes MT19937 raw draws; disabled mode retains thread-local SFMT. Existing core wrappers consume the shared test stream.
- Random.TestSeed.Enabled=0, Value=1 documented in conf.dist; independent test profile overlay enables seed 1. Initial seeding runs before startup config callbacks can generate bots. Reload reconciles configured state without rewinding an unchanged enabled stream.
- No build-speed work, Heartseed edits, ScriptMgr.h changes, schema/database changes or new dependencies.

## How it was tested
- MSVC Release unit_reload_registry and unit_seeded_random passed with no warnings: validation/publish retention, name bounds, duplicate/unknown/exception/reentrant/concurrent reloads; seed zero/max/parse rejection, repeat/changed seed, disable/status/count and concurrent global stream consumption.
- Tests use exact T-008 framework copies in ignored build/reload-unit-integration; no branches stacked. After PR #13 merges they register automatically from tests/reload/.
- Actual CustomReload.cpp and Random.cpp compiled to standalone objects using existing build include settings. No errors; inherited assertion pragma/conversion warnings are in existing headers/unchanged legacy RNG lines, not new helper/module code. No server link/full build.
- Prepared Heartseed manager and command adapter compiled as two isolated objects against Claude's T-022 headers; no full build. The staged loader rejects failed queries/invalid rows before swapping its cache. These owner changes are not yet in this PR: integration waits for playerbot to include the owning module.
- Added tests/regression/scenarios/custom-reload-rng.json for registered Heartseed names, uint32 boundaries, malformed seeds, missing table failure, off and config reconciliation. Live execution is pending the merged/rebuilt server.
- Full startup, RBAC, command merge/GM/SOAP and real owning-table cache checks are NOT claimed. README has the post-build checks. PR #8 remained OPEN at final check, so no full build started.

## Decisions and limits
- The seed controls core Random.h APIs; direct C rand, independent std generators and Lua RNG remain outside that interface. Raw stream repeatability does not promise scheduling-independent whole-server replay; fixture/call order and toolchain must match.
- Framework supplies registration/dispatch; owner callbacks stage/validate their own SQL/cache data. Current request explicitly authorizes registering Heartseed after its pending owning PRs merge.

## Map additions
- src/server/scripts/Custom/Reload/: registry, command/config adapter, registration docs.
- src/common/Utilities/DeterministicRandom.h and RandomSeedState.h: portable test stream/status.
- tests/reload/: pure unit suites.
