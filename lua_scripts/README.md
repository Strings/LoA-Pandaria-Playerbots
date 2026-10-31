# Lua prototypes

Eluna is already compiled into the existing server. This task enables it only
in generated dev/test profiles; shipped worldserver.conf.dist stays unchanged.
The profile.d/009-eluna.json overlay is consumed by T-002's setup generator once
both independent PRs are merged. To enable an existing private runtime now:

```
python tools/devenv/enable_eluna.py build/devenv
```

Restart worldserver the first time. Subsequent edits reload with `.reload config`
(also through SOAP); this core calls StartEluna(true) there when Eluna is enabled.
This rebuilds the Lua state and registrations, not C++ scripts. Avoid changing
the whole server config merely to test a live content edit unless you own it.

Layout:

- 00_bootstrap.lua: shared UltimateMoP namespace, startup marker.
- examples/: harmless opt-in probes; GM chat `prototype-check` returns a confirmation.
- heartseed/: future owning-task dialogue/scenario prototypes.
- Add each other feature in its own named folder, with purpose/config/GM hooks.

The loader recursively loads .lua files into one state, ordered by path. Keep
cross-file dependencies explicit; do not rely on filesystem traversal order.
Use this fork's LuaEngine headers as the API reference (older WotLK Eluna scripts
can differ). Keep secrets out of logs. Pure rules should get independent tests;
timers belong in configuration, and persistent state belongs in owning-system
custom tables. Prototype rares, events, gossip and dialogue here before C++.

Build-free smoke: start the private none/test runtime, log a GM in through the
headless client with `--cmd play --hold 1 --step "say prototype-check"`, and assert
`UltimateMoP prototype hooks ready.` in its output. Run `reload config` over SOAP,
then repeat; one confirmation per chat proves the hook was not duplicated.
