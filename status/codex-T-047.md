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
