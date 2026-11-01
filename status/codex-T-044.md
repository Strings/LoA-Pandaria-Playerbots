BLOCKED: May I publish this own branch with force-with-lease after rebasing? Automatic approval review rejected it; build/live verification continues on current published history.

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

## Next / exact native check plan
- After T-041, build this branch in the primary checkout through the normal queue. Apply world 04/auth 05 SQL to Codex DBs before its server startup; no main SQL until approved/merged.
- Use a separate normal-security owned account and level-15 Orc fixture (preset while offline). Verify `.xp off/on/status` is available through Player Commands role 199; selection of an online GM still changes/inspects self. GM target permission 95001 must force/inspect that fixture.
- Record money before/after toggles; repeated requests must be free and idempotent. Relog and check the native 0x02000000 player flag persists.
- Keep the normal fixture online; selected-player GM `.quest add`, `complete`, `reward` for current MoP Ragefire quests 30983/30984 exercises real GiveXP. With XP off, level/XP stay identical; with XP on, total progress increases. Use a clean owned fixture, not another player's quest history.
- Queue/leave a normal battleground in each XP mode and inspect native queue acceptance; code audit already shows no XP-specific queue dimension.
- Preset the owned fixture beside existing Experience Eliminator 35364 while offline, then use native gossip: enabled path changes the flag with identical money; disabled path uses native fee/text. Restore key on and clear the test queue.
- Run all three regressions; write actual results, mark review/ready, wait for Claude approval before routine merge. No new duration.
