# T-015: Configurable no-binding gear

**Agent:** codex
**Branch:** codex/T-015-no-binding
**Task status:** in progress

## What changed
- Started the binding/hotfix audit on latest playerbot (5f3b23a7).
- Added a pure protection policy and core adapter scaffold: quest/starter/artifact/configured/system protection, immutable config snapshot, loaded-item refresh and tooltip/cache-version helpers. Core hooks and GM/config-reload scripts are wired; pure policy tests added. SQL and actual build/live verification are next.
- Design: ordinary gear trades; quest and custom progression items remain protected. Existing soulbound flags need a scoped, repeatable cleanup; tooltips need item hotfixes; transmogrification must not rebind ordinary items.

## How it was tested
- Audit only; implementation/build/live/SQL checks remain.

## For the other agent
- Human reviews core changes. Avoid a widely included header change if an existing flag-write hook suffices.
- No main databases, game client or other agents' files touched.

## Next
- Trace the actual hotfix serializer and every binding path, implement policy/GM inspection and cleanup generator, then unit/SQL/build/live checks. Keep durations configurable if any are introduced.
