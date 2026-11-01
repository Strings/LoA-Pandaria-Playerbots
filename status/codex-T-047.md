# T-047: Multiple tracking types

**Agent:** codex
**Branch:** codex/T-047-multi-tracking
**Task status:** in progress

## What changed
- Started on latest playerbot, after T-044's completed verification and T-046's full 11-scenario pass (final latest-base build follows).
- Read the provisional multi-tracking design. This fork already ORs/removes creature/resource bits in SpellAuraEffects.cpp; no tracker-specific exclusion appears in SpellInfo/SpellAuras, and the four probe spells have no explicit spell-group exclusion in world_codex.
- Test actual native aura combinations before adding any core change. Keep the client-test requirement: several minimap types and dropdown checkmarks. GUI permission is not granted yet.

## Next
- Lua prototype/VM checks, bounded GM force/inspect probe behind a config key, native mask/add/remove controls. Add core behavior only if the baseline proves necessary.
- Build and relevant units/live/regressions. Visual client check needs explicit permission under AGENTS.md; never launch or write into mop-clean.
- Record that the design is provisional. Status/task review and one PR; Claude reviews before routine merge, human reviews any core touch. No duration planned.

## Map additions
- Planned Custom/Tracking/ and tools/tracking/ diagnostic/prototype.

## Lua probe checkpoint
- Added an optional Eluna GM probe in lua_scripts/Custom/Tracking, with Tracking.Probe.Enable=0 documented in conf.dist. It reads the active config (MOP_WORLD_CONFIG override), forces only four whitelisted self auras, inspects both native masks, and allows cleanup while disabled. No core behavior, database IDs or duration added.
- Six Lua VM tests pass: disabled/missing config, GM guard, resource-only profile, no-mutation status/invalid input, unrelated-command passthrough, cleanup after disable. Production probe remains Lua because native aura APIs suffice.
- This is still provisional. Next: deploy to Codex-only Eluna runtime, prove actual combined bits/add/remove/key controls, then request permission for the required client visual test. Server masks alone cannot prove client rendering/checkmarks.
- Map additions: lua_scripts/Custom/Tracking/ and tools/tracking/.
