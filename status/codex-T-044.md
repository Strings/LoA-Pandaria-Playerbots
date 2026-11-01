# T-044: Free experience toggle

**Agent:** codex
**Branch:** codex/T-044-xp-toggle
**Task status:** in progress

## What changed
- Started from latest playerbot a273234b; T-014's build is still queued, so this is independent preparation in the secondary Codex checkout.
- Read the decided twinking design. Audited core no-XP references: XP gain observes the flag, but queue/group/handler paths do not segregate it. No queue core change is needed.

## Implementation (pushed, not yet built/live-tested)
- Public self-only `.xp on|off|status`; separate GM target permission 95001 enables force/inspect of the selected player. Uses and persists the native XP flag, with no money changes and idempotent repeated requests.
- `Twinks.FreeXPToggle=1`, `Twinks.IntroOnLogin=1`; no new timer. Existing NPC calls the shared adapter when enabled; native paid code and text remain the disabled fallback.
- Small NPC/loader call sites, own module, pure parser suite, custom free greeting and scoped auth permission SQL. SQL validator passes both files with zero issues.
- Reserved the next Twinks block 95000; includes pending T-041's preceding reservation from PR 85 to avoid overlap.

## Verification so far
- All 15 unit suites pass, including the new parser; no warnings in its build.
- Both SQL files pass lint (zero issues), apply twice identically on copied real NPC/auth rows in world_codex_t044_68cd9d80, and preserve every original row. Runtime/main DBs unchanged.
- Full production build/live/regressions remain pending behind T-014/T-041 and the shared build queue. No claim of ready.

## Next
- Public self-only toggle/status, selected-player GM permission, config and brief login introduction.
- Route Experience Eliminator NPCs through the free adapter when enabled; preserve paid native behavior when disabled.
- Pure rules tests, auth SQL, build/live/regressions, final status/review. T-014 live checks remain first when the compiler is free.
