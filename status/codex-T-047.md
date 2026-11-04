BLOCKED: The human reports the client is crashing. Claude owns crash fixes; the human explicitly asked Codex not to fix it. Resume the client tracking observation once the client is usable.

# T-047: Native multiple-tracking probe

**Agent:** codex
**Branch:** codex/T-047-multi-tracking
**Task status:** blocked pending the client observation required by handoff step 2

## What changed
- Rebased PR 99 on current playerbot 9a656fb4. This is the provisional multi-tracking experiment from DESIGN.md; no core stacking rule is changed.
- Finished the opt-in GM Lua probe, disabled by default. Whitelisted triggered self-casts use the native tracker auras; inspection reads the actual private tracking fields. Cleanup remains usable when disabled. No spells are learned, IDs allocated or durations added.
- Corrected unsupported Lua API assumptions: this fork disables AddAura and HasAura bindings. Use registered CastSpell(target, id, true) and mask inspection. A new compatibility test checks every player method used against the actual enabled LuaFunctions.cpp registry.

## Verification
- Eight Lua VM tests PASS, including config/missing config, GM guard, native event 18 registration, registry compatibility, isolated resource profile, invalid/status input, passthrough and disabled cleanup.
- Live Codex headless checks PASS against the current playerbot binary with PR 91's additional travel adapter. Lua scripts under test are this branch's exact scripts; no C++ is changed by PR 99.
- Native all: creatures=65, resources=6. Removing Track Beasts: 64/6. Removing Find Herbs: 64/4. Resources profile: 0/6; creatures profile: 65/0; clear: 0/0. Assertions verify the complete sequence in build/T047-native.log.
- Forcing while disabled is rejected; inspection and cleanup remain available. Ordinary-player forcing is rejected. Logs: build/T047-disabled.log and build/T047-non-gm.log.
- GUI/client rendering and dropdown checkmarks have NOT been observed. Server masks are not proof of those behaviors.

## Exact human client test
1. Use a disposable GM hunter on the Codex test realm (auth 3744, world 8105). The probe is installed in build/T046-current-runtime and its private forcing key is enabled. Do not test on a character whose current tracking selections matter.
2. If needed on that fixture only, learn the four abilities: `.learn 1494`, `.learn 19883`, `.learn 2383`, `.learn 2580`. Their names are Track Beasts, Track Humanoids, Find Herbs and Find Minerals.
3. In ordinary Say chat, without a leading dot or slash, send `trackingprobe clear`, then `trackingprobe all`. The status must read creatures=65 resources=6.
4. Open the minimap tracking dropdown. Record which of the four abilities have checkmarks. Near known beasts, humanoids and herb/ore nodes, record which marker categories appear together. Visit suitable nearby spots if one location lacks all categories; confirm each category exists there before judging missing markers.
5. Click one tracking option off in the dropdown and run `trackingprobe status`. Record the checkmarks, markers and both masks; this tests whether the client UI clears other selections.
6. Send `trackingprobe clear` afterward. Remove only spells learned solely for this disposable test if desired.

Question: do all four checkmarks coexist, do both categories of creature markers and both resource markers render, and does changing one option preserve the others? Report actual observations, including any category the location could not test.

AGENTS.md requires asking before computer use. The handoff explicitly asks for human steps and a blocked task when this observation is needed, so no GUI was launched. PR 99 remains a draft. Continue T-007 while waiting.

## Map additions
- lua_scripts/Custom/Tracking/: opt-in native tracking diagnostic and instructions.
- tools/tracking/: Lua VM/API compatibility tests.

Human update (2026-11-04): client currently crashes; crash investigation may be read-only, fixes belong to Claude. No GUI or crash fix attempted. Continuing T-007.
