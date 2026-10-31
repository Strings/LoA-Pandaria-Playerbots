# T-038: Custom-table reload and core test RNG

**Agent:** codex
**Branch:** codex/T-038-reload-rng
**Task status:** review (draft; new work stopped at the human's request)

## What changed
- Custom/Reload provides a server-independent callback registry and command/config adapter: `.reload <table>`, `.reload custom <table>` and `.reload custom list`. Built-in reload commands retain precedence; ScriptMgr.h and cs_reload.cpp are unchanged.
- RegisterGroup validates aliases together and shares a loader/guard. Invalid/duplicate names, overlapping/reentrant reloads and loader exceptions are rejected. Owners stage and validate data before publishing.
- Heartseed startup and `.heartseed reload` use one registration covering heartseed_stage, heartseed_glow, heartseed_trait, heartseed_essence_template, heartseed_essence_source, twink_item and heartseed_shrine, plus the heartseed bundle alias. Failed queries or invalid data retain the complete prior cache; empty successful queries are distinguished from failures.
- Common Random.h APIs SetRandomSeed/ClearRandomSeed/GetRandomSeedState and `.rng seed <uint32>`, `.rng off`, `.rng status` provide a shared MT19937 test stream. Disabled mode retains thread-local SFMT.
- Random.TestSeed.Enabled=0 and Value=1 are documented; the test profile enables seed 1. Initial seeding precedes startup config callbacks. Reload does not rewind an already matching configured stream.
- Rebased onto playerbot including the merged checkpoint work; preserved its script registration and Heartseed behavior. No build-speed work, new dependencies or schema changes.

## Completed validation
- Actual merged T-008 framework: MSVC Release CTest passed unit_time_value, unit_reload_registry and unit_seeded_random. Registry coverage includes retained snapshots, alias validation, exceptions, reentrancy and concurrency; RNG coverage includes zero/max seeds, parsing, reset/disable/count semantics and concurrent consumption.
- Full Release auth/world build passed (67.8 minutes), followed by the integration build after rebasing onto the checkpoint/twink batch (1.3 minutes). No new warnings in touched files; inherited auth conversion and generated version-header warnings remain.
- These builds included a separate, uncommitted T-019 validation overlay. They are integrated build evidence, not a claim that the T-038 branch alone was rebuilt without that overlay.
- Private test/none startup succeeded with merged Heartseed/checkpoint migrations applied only to Codex databases. SOAP `.rng status` returned enabled seed 1; built-in `.reload config` returned HTTP 200.

## What remains
- playerbot advanced again with the T-049 account-wide Heartseed work after this rebase/build. Rebase onto that newer batch and resolve any Heartseed integration conflicts before final validation.

1. Run tests/regression/scenarios/custom-reload-rng.json and the login regression suite on the final branch build.
2. Verify GM/SOAP registration listing, both custom reload forms, built-in command precedence, unknown-name failures, and ordinary-account permission denial.
3. Exercise valid Heartseed data changes through the registry and legacy `.heartseed reload` alias. Inject invalid owned test data and a controlled query failure; verify failure reporting and retention of the previous cache, then restore the data.
4. Live-check seed zero/max/repeat/off and malformed inputs. Confirm unchanged config reload preserves draw progress, explicit reseed resets it, and a controlled fixture/call order repeats outcomes.
5. Complete the live checks before marking PR #15 ready or merging it. Reconcile the older verification wording in Custom/Reload/README.md when work resumes.

## Handoff boundaries
- The private runtime is build/devenv-solo-reload on Codex ports 3744/8105/7898; its process IDs are recorded in ignored build/validated-private-server-pids.json. It includes T-019 test code/settings.
- SoloConfig files, Player.cpp/World.cpp edits and the SoloConfig ScriptLoader registration are T-019 work. They remain outside T-038 commits; do not stage the whole working tree.
- Global test seeding covers core Random.h APIs, not direct C rand(), independent std generators, Lua or cryptographic RNGs. Raw-stream repeatability does not guarantee whole-server replay independent of scheduling.

## Map additions
- src/server/scripts/Custom/Reload/: registry, commands/config adapter and registration docs.
- src/common/Utilities/DeterministicRandom.h and RandomSeedState.h: portable test stream/status.
- tests/reload/: unit suites; tests/regression/scenarios/custom-reload-rng.json: pending live feature scenario.
