# T-044: Free XP toggle

**Agent:** codex
**Branch:** codex/T-044-xp-toggle
**Task status:** review; ready PR 86

## What changed
- Public .xp on/off/status changes only the caller, for free; GM permission enables selected-player force/inspection. Parser rejects extra/invalid input. Repeated changes are no-ops, native no-XP flag persists.
- Twinks.FreeXPToggle=1 and Twinks.IntroOnLogin=1. Existing Experience Eliminator gossip uses the same adapter; key off restores the native 10g service/text and rejects command changes while allowing inspection.
- No XP-off queue dimension exists in queue/group/join code; no core-game queue change needed. No new duration. Module/README, config, script registration, player/GM RBAC and free greeting SQL included.

## Verification
- Original 15 unit suites/parser passed; policy unchanged after rebase. Existing XP test binary rerun exits 0. Current-base unit compilation/testing is queued behind the same Local\UltimateMoP-build mutex as production; no overlap bypass.
- Both SQL files lint clean, apply twice identically to copied real NPC/auth rows, preserve originals. World 04/auth 05 SQL now applied only to world_codex/auth_codex; main unchanged.
- Rebased on playerbot 2c654e81 (merged T-014) and published cc9eb08d after explicit user approval for force-with-lease on remaining rebased Codex branches. Approval block is cleared.
- Default production build queued through tools/build/build.ps1 -Target worldserver -Reconfigure. Log build/T044-build.log, tool session 39850. Unit log build/T044-unit-queued.log, session 47572. Do not alter C++ inputs while queued/running.

## Next / native check plan
- After build success, copy the binary to a unique snapshot in build/devenv-solo-reload and start hidden with its own worldserver.conf. Auth stays on 3744; world/SOAP 8105/7898, only Codex DBs. World is currently stopped.
- Ignored build/T044-live.py creates owned normal account CODEXXP and Orc Codexxp, presets level 15/quiet map-1 position while offline, and tests public self-only permissions with another player selected, money/idempotence/relog, selected-player GM force and real quest XP off/on (30983/30984). Put the visible root GM at the same quiet position before the selection check.
- Private profile uses existing PlayerSaveInterval=2000 for observable live XP persistence; restore 60000 after checks. Profile was restored from validated Codex config after an encoding error; own ports/DBs verified. Always write as UTF-8.
- Ignored build/T044-extra.py tests normal WSG queue acceptance/leave in each XP mode, then real Experience Eliminator 35364 gossip: enabled is free; disabled command rejects, stock NPC charges exactly 100000 copper per change. Restore key on and clear test queues.
- T-041-owned runtime NPC/spell bindings were removed before switching; no other agent DBs/ports touched. Its PR85 is ready (build, 15 units, nine regressions).
- Run all current headless regressions; final status/task review and mark ready. Claude approval required before routine merge. Apply these SQL files to main only after merge.

## Map additions
- src/server/scripts/Custom/Twinks/ with reusable SetExperienceEnabled adapter; tests/twink_xp.

## Completed verification checkpoint
- Current-base normal-queue production build passes (build/T044-build.log), no touched-file compiler warnings. Same-mutex current CTest build/run passes all 16 suites (build/T044-unit-queued.log).
- Native ordinary-player permissions/self-only selected-GM control, unchanged money, repeated toggle/relog, selected-player GM force and actual quest XP off/on all PASS. Native NPC/queue controls and full regressions next.

## Final results
- Normal WSG queues accept both XP modes; both test queues cleared. All four native NPC switches pass: enabled costs zero; disabled command rejects and native NPC charges exactly 100000 copper each way. Key restored on.
- All 11 current headless regressions pass, including newly merged mountspeed/solo scenarios. Report in primary checkout: build/regression-T044/20261101T201936Z-0b9448e3/results.json.
- PlayerSaveInterval restored to original 60000, missing current default keys appended without replacing private overrides. No new duration, no main DB changes. Claude approval before routine merge; after merge apply only world 04/auth 05 SQL to main.
- Build and 16-unit-suite pass precede only documentation commits. Native GUI unused. T-046 compilation/live verification follows in primary; this branch is in the secondary Codex checkout for review updates.
