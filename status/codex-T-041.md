# T-041: Script quality-of-life changes

**Agent:** codex
**Branch:** codex/T-041-solo-qol
**Task status:** in progress

## What changed
- Started script half on latest playerbot; Flash's material-stack data is excluded.
- Reserved the next documented SoloQoL ID block for a character-services steward and GM permission.
- Audited existing mixed armor/weapon transmog settings: both already ship enabled, as do legendary transmog and most requirement bypasses. Verify/expose these rather than add duplicate core logic.
- Guild-removal script hook runs before native perk removal: defer reconciliation through a dirty-player queue on the next world update, without a new timer.

## Next
- Prototype free character-services gossip in Lua, then production script/SQL.
- Grant/reconcile guild perks; inspect current cooldown APIs for a minimal configurable Hearthstone override and GM hooks.
- Pure logic/unit/SQL/build/live/regression checks. Every duration is a config key.
- T-014 build/live verification takes priority as soon as the shared compiler becomes available; primary checkout stays on T-014.
