# Custom-table reload and test RNG

The pure Registry in ReloadRegistry.h owns registered names/callbacks, not game
data or SQL. CustomReload.cpp adapts it to existing CommandScript/WorldScript
APIs. No ScriptMgr.h hooks are added. Reconfigure before
the eventual server build so the existing Custom source glob sees this module.

## Register a table in its owning module

Include `Custom/Reload/ReloadRegistry.h` and register once in AddSC_<system>:

```cpp
bool registered = CustomReload::Tables().Register("custom_traits", "Trait definitions", []() -> CustomReload::Result
{
    auto next = LoadAndValidateTraitSnapshot(); // Owning module supplies this.
    if (!next)
        return { false, 0, "Validation failed; active snapshot retained" };
    auto rows = next->size();
    PublishTraitSnapshot(std::move(next));      // No-throw atomic publication.
    return { true, rows, "Published new snapshot" };
});
```

Check registration failure and report duplicate/invalid names through the owner's
log channel. Names are lowercase SQL-style identifiers, max 64 characters;
custom/list are reserved command words. Register callbacks at script startup,
not per login/config reload, and capture objects with server lifetime. There is
no unregister/replacement operation that could invalidate in-flight callbacks.

Loader contract: query and validate into a private snapshot first, including
foreign IDs, ranges and cross-table invariants. Distinguish an allowed empty
table from failed loading according to system policy. Publish only after full
success, with a no-throw swap/atomic shared-pointer replacement; readers hold a
snapshot so old generations remain alive. Failure/exception must retain the old
snapshot. The registry catches exceptions but cannot undo a loader that mutates
live data prematurely. Related tables can share one validated snapshot loader.
Use `RegisterGroup({ "table_a", "table_b" }, description, loader)` when related
tables must reload together. Names in one group share a callback and busy guard;
invalid or duplicate names reject the entire registration. Reloading any member
publishes the whole validated group, with the group's total accepted row count.
Callbacks run synchronously on the command caller (normally the world thread);
do not wait for work that requires that thread. Same-table overlapping or
recursive reloads fail as busy; unrelated registration/listing does not hold an
I/O lock. Registry metadata is thread-safe, but each owning cache still needs
its own reader/publication policy.

Gameplay adapters belong to their owning systems. The unit fixture provides a
real staged/published callback without introducing a dummy world database table.

## GM / SOAP commands

- `.reload custom list`: registered names/descriptions.
- `.reload custom <table>`: exact canonical custom-table dispatch.
- `.reload <table>`: fallback dispatch when no built-in child command matches.
  Existing built-ins and their abbreviation rules retain precedence; use the
  canonical custom form if a table name overlaps a built-in prefix.
- `.rng seed <uint32>`: reset and enable the process-wide core test stream;
  zero and 4294967295 are valid seeds.
- `.rng off`: resume normal thread-local SFMT streams.
- `.rng status`: mode, last seed and raw draw count; reads consume no draw.

All operations require the existing reload-config RBAC permission and allow
console/SOAP. Unknown names, loader failures and malformed seeds report failure
with SetSentErrorMessage, so SOAP cannot count them as successful commands.
The registry is an explicit allowlist; commands execute no arbitrary SQL.

## RNG scope and configuration

Random.h's SetRandomSeed/ClearRandomSeed/GetRandomSeedState are common-library
APIs. Test mode serializes raw MT19937 draws across all threads; rand32,
RandomEngine, urand/irand/frand, randtime, weighted draws and core chance helpers
all consume this shared stream. Production retains thread-local SFMT when off.
RandomSeedState.h is lightweight; the <random>/mutex implementation is not added
to every Random.h includer. Cryptographic RNGs are unaffected. Direct C rand(),
independent std generators and Lua's RNG are outside this API's scope.

worldserver.conf.dist has Random.TestSeed.Enabled=0 and Value=1. The independent
T-002 profile overlay enables Value=1 for test only. On config load, invalid seed
text logs an error and retains the prior state. Reload reconciles configured
mode/seed without resetting an already matching enabled stream. Initial seeding
happens during script registration, before other WorldScripts generate bots in
their initial config callbacks. Manual GM
overrides last until config is reconciled or the process exits. Re-seeding the
same value through the GM/API explicitly resets raw draw count/sequence.

An identical seed gives an identical raw stream. Distribution mapping can vary
with the standard library, and thread/tick ordering determines which system
receives each draw. This alone does not promise whole-server replay: pin fixture
state, call order and scheduling, use the same toolchain, reseed at scenario
start, and route relevant rules through the core API.

## Verification and integration boundary

tests/reload registers two T-008 CTest targets: unit_reload_registry and
unit_seeded_random. They cover retained snapshots on invalid data, successful
publication, invalid/duplicate/unknown names, exception/reentrant/concurrent
reloads, seed boundaries/parsing, reset/disable/count semantics and concurrent
consumption of one stream. No database or server is needed.

After T-008 merges, configure `cmake -S tests -B build/unit-tests`, build the two
named targets, then run CTest. These tests were already compiled in Release
against exact copies of T-008's framework in an ignored integration fixture;
no task branches were stacked.

Pending on the rebuilt private test/none server:

1. Confirm `.reload config` and other built-in commands still dispatch normally.
2. Confirm ordinary accounts cannot invoke custom reload/RNG; GM/SOAP can list
   the registry, unknown tables fail, and a failed loader produces SOAP failure.
3. Once an owning table adapter registers, edit valid data and reload via both
   forms; observe the new cache. Invalid data must retain the prior generation.
4. Check `.rng seed 0`, maximum uint32, repeat seed, status and off; reject
   negative/overflow/trailing input without changing the prior state.
5. With a controlled fixture/call order, repeat a seeded scenario and compare
   outcomes; verify disabled mode still works. Reload unchanged configured seed
   must not rewind the draw count, while explicit reseed must rewind it.

No server link/startup/GM integration result is claimed yet. Full builds remain
deferred while Claude's PR #8 is open; build-speed changes are outside this task.

The feature regression plan is `tests/regression/scenarios/custom-reload-rng.json`.
It needs merged Heartseed data/registration and a regenerated test/none runtime
with seed 1, plus dedicated GM/SOAP credentials. It ends by reconciling config.
