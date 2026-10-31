BLOCKED: approve the numerical heirloom margin (proposed 5% primary stats, stamina and weapon DPS; leave stronger gear unchanged).

# T-045: Twink gear research

**Agent:** codex
**Branch:** codex/T-045-twink-gear
**Task status:** blocked

## What changed
- Added docs/twinks.md: 1,711 raw item-level candidate rows across 14 bracket endpoints/breakpoints and the reproducible read-only query.
- Documented acquisition limitations and the template/hotfix and upgraded-item stat allocation paths.
- This is a research shortlist, not verified per-spec BiS. No balance SQL applied.

## How it was tested
- Executed the query on imported world_codex (80,064 templates); quest-only zero-level rewards require a qualifying quest minimum.
- Inspected ObjectMgr::LoadItemTemplates, Item::GetStatValue and SendItemSparseDb2Reply.
- No full build or in-game check; task remains incomplete.

## Next
- Resolve numerical balance target, verify per-spec acquisition and effective heirloom curves, generate absolute-value idempotent SQL, validate server stats and client hotfix delivery.

## For the other agent
- No shared database changes. No build started.
