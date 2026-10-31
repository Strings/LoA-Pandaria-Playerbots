BLOCKED: Preserving unfinished raids needs a larger per-instance reset/lifecycle change. Approve that scope and define abandonment before implementation.

# T-013: Raid reset and retention audit

**Agent:** codex
**Branch:** codex/T-013-raid-resets
**Task status:** blocked

## Findings
- InstanceSaveMgr::LoadInstances deletes expired raid saves at startup using the map/difficulty-wide instance_reset deadline, without checking progress.
- _ResetOrWarnAll unbinds every loaded save and batch-deletes character binds, group binds and instance rows for the entire map/difficulty. InstanceMap::Reset(INSTANCE_RESET_GLOBAL) then invalidates occupants and discards state on unload.
- Changing the reset period alone would still wipe unfinished raids. Safe retention must cover loaded/unloaded saves, restarts, player/group binds, completion detection and an abandonment operation. This exceeds a period/config quick win.
- DESIGN.md calls the retention reading provisional; this task explicitly requires it, but does not define abandonment.

## Question
Approve a larger persistent per-instance retention implementation? Proposed meaning: preserve a bound, partially cleared raid across empty periods and restarts until all encounters are complete or its owner explicitly abandons it. Define who may abandon shared progress (leader vs every bound player) before changing existing save lifecycle.

## Verification
Read-only inspection of InstanceSaveMgr.cpp, InstanceMap::Reset, and DESIGN.md lockouts. No executable/config/SQL changes; build and live reset tests are intentionally deferred until the scope is settled. No shared databases or processes touched.

## Next
Continue T-011 and T-016 independently. Do not ship a two-day timer alone as if it satisfied retention.
