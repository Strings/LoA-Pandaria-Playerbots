# Native multiple-tracking probe (T-047)

The provisional design needs an actual client test. This fork already adds/removes tracking bits and has no tracker exclusivity rule for these spells. This GM diagnostic uses native auras; it changes no gameplay stacking rule.

Enable Eluna and install this folder under its `Eluna.ScriptPath`. Set `Tracking.Probe.Enable=1` in the active world config for forcing, then `.reload eluna`. The Lua probe reads that key per command; `MOP_WORLD_CONFIG` overrides the default `worldserver.conf` in the server working directory when `-c` uses another file. No credentials are printed. Default forcing is off; native player tracking is unchanged.

GM rank 2+ can use `.trackingprobe status|resources|creatures|all|clear` on self. Profiles replace only the four probe auras: Track Beasts 1494, Track Humanoids 19883, Find Herbs 2383 and Find Minerals 2580. Status works while disabled; clear remains available for cleanup. Use a dedicated fixture, since these four existing auras are replaced. No spells are learned, no table/permission IDs or durations are added.

Expected native masks: creatures 65 (beasts plus humanoids), resources 6 (herbs plus minerals). Confirm removal leaves the other category intact through ordinary aura changes, and disabling the probe rejects forcing. Visual test: a hunter who knows these tracking abilities near both creature types and herb/ore nodes; check simultaneous markers and multiple dropdown checkmarks. Do not claim that test from server masks alone. GUI permission is required; never launch or write into `mop-clean`.

VM guard/config/dispatch tests: `python -m unittest discover -s tools/tracking -v` (uses the existing development-only `lupa` package). Native/client results belong in `status/codex-T-047.md`.
