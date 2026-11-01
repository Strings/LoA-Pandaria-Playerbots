# T-044: Free experience toggle

**Agent:** codex
**Branch:** codex/T-044-xp-toggle
**Task status:** in progress

## What changed
- Started from latest playerbot a273234b; T-014's build is still queued, so this is independent preparation in the secondary Codex checkout.
- Read the decided twinking design. Audited core no-XP references: XP gain observes the flag, but queue/group/handler paths do not segregate it. No queue core change is needed.

## Next
- Public self-only toggle/status, selected-player GM permission, config and brief login introduction.
- Route Experience Eliminator NPCs through the free adapter when enabled; preserve paid native behavior when disabled.
- Pure rules tests, auth SQL, build/live/regressions, final status/review. T-014 live checks remain first when the compiler is free.
