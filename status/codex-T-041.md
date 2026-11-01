# T-041: Script quality of life

**Agent:** codex
**Branch:** codex/T-041-solo-qol
**Task status:** review; ready PR 85

## What changed
- Script half only; Flash owns stack data. Rebased on playerbot aa0af9db.
- All 26 native guild perks reconcile on login/config reload and after native guild join/leave, without a polling timer. Disabling restores membership/level rules.
- Free city stewards 904000/904001 offer rename/customize/race/faction flags, preserving unrelated flags. Bots, neutral Pandaren and combat are excluded. Lua prototype preceded the C++ adapter; two VM tests cover flags/guards.
- Hearthstone AfterCast applies the configured final cooldown through existing spell/category APIs. Zero clears, negative retains native behavior. Normal casts work even if Hasty Hearth erased the native cooldown; triggered casts are excluded. Durations/respawn are config keys, default 300 seconds.
- Existing transmog module rules are reused. One ItemHandler.cpp eligibility call site, behind SoloQoL.NativeTransmogRules, makes the native window honor them after existing interaction/ownership/slot checks. The module's legacy config hook is undispatched by this core, so SoloQoL refreshes it during config reload.
- GM .soloqol status/perks/hearth/hearthreset/service, permission 94000. Login introduction, README, scoped world/auth SQL and ID reservation included.

## Verification
- Default normal-queue production build passes (build/T041-build-refresh.log); no new compiler warnings in touched files. No queue bypass used.
- Current-branch 15 CTest suites pass (Release), both Lua VM tests and tools/devenv/test_solo.py pass.
- Both SQL files lint clean. Applied twice to real-source-row copies with identical second result and originals preserved. Corrected fixed respawn SQL rechecked on world_codex_t041_d9948725. Applied to world_codex/auth_codex only; main unchanged.
- Native Hearthstone item cast: configured 60 seconds, feature disabled, zero, negative/native retention and GM force/reset pass.
- Guildless 26/26, off/on, owned guild create/disband and reconciliation pass. Test guild removed.
- All four real NPC gossip choices set expected flags 1/8/128/64 without charging money; disabled/neutral guards pass, flags restored. Bot/combat guards covered by policy tests. Restarted NPC inspection reports configured Full Min 2s / Full Max 2s (spawn max sentinel 0).
- Native cloth-on-plate and mace-on-sword requests succeed with mixed settings on and are rejected with each setting off; stored appearance checked and cleared. NativeTransmogRules=0 retains stock rejection. Actual loaded settings inspected.
- Runtime durations restored to shipped 300/300; mixed settings and core bridge restored on. Native game GUI was not launched.

## Next / handoff
- Full current headless regression suite passes: all nine scenarios (capture, checkpoints, reload/RNG, GM tools, Heartseed, loadouts, login, reagent bank, respec). Report: ignored build/regression-T041/20261101T191050Z-690a36fe/results.json.
- This PR touches core: Claude review plus human merge required. Do not self-merge. Apply scoped SQL to main only after merge.
- T-013/PR45 and T-011/PR61 have Claude approval with no required fixes, held for human merge. T-014/PR82 and T-015/PR69 are ready. Next verification is T-044/PR86, then T-046/PR91.

## Map additions
- src/server/scripts/Custom/SoloQoL/ and tools/soloqol/; native item-hotfix/transmog helpers in contrib/headless-client/headless.py.
