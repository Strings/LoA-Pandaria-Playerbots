# T-015: Configurable no-binding gear

**Agent:** codex
**Branch:** codex/T-015-no-binding
**Task status:** in progress (draft PR 69; not ready to ship)

## What changed
- Rebased on playerbot 6bbfb8ef, including the merged routine regression harness fix (PR 67).
- Pure BindingRules and game adapter protect quest/quest-starter/artifact/configured entries, unknown templates and registered per-instance progression checks.
- Minimal core call sites cover soulbound flag writes (including transmog), loaded bindings, binding enchants and item-sparse tooltip flags/bonding. Widely included headers stay unchanged.
- Config reload refreshes online inventory and hotfix versions; metadata is batched below packet-size limits. GM status/item/bind/refresh commands and brief login intro exist.
- Cleanup generator consumes an explicit numeric TSV catalog, supports protected entry/GUID lists and emits scoped soulbound-bit updates plus non-destructive hotfix metadata SQL. Original template binding remains intact.

## How it was tested
- Default RelWithDebInfo worldserver build passes with no new warnings in touched files (build/T015-first-build.log).
- All 12 CTest suites pass, including protection, tooltip flags, version monotonicity and metadata packet boundaries.
- All six cleanup-generator unittests and Python syntax checks pass.
- Generated world hotfix and character cleanup updates from 80,064 templates (71,870 eligible); SQL validator reports zero issues.
- Verified twice on disposable world/real-character-save copies: all 151 item rows including five sentinel types match the expected flag-only change, configured entry/GUID exclusions work, unknown/quest/artifact items remain bound, and every other character table hashes identically. Both updates then applied only to Codex DBs.
- Final build and all 12 units pass again. Live and regression checks are now running against the new binary.

## Decisions made
- Config defaults: NoBinding.Enable=1, NoBinding.ProtectedItemIds="", NoBinding.IntroOnLogin=1. No durations introduced.
- Artifact quality is conservatively protected. Future progression/storage systems register instance protection before player loading, and export protected low GUIDs when generating a one-time cleanup.
- Disabling restores future binding/tooltip behavior, not historical flags cleared by an explicit cleanup.

## For the next session
- Current primary worktree is on this branch, clean and pushed. Secondary build/pr-validation is detached; do not edit it for this task.
- Codex server currently runs the matching baseline/harness-fix snapshot worldserver-regression-fix.exe in build/devenv-solo-reload, not the new no-binding binary. Auth 3744, world 8105, SOAP 7898; *_codex DBs only. Random bot AI disabled. Main fixture is guid 1, safely staged with GM on/god off. Private credentials remain under ignored build/runtime.
- T-013 PR45 and T-011 PR61 are ready, unmerged for human core review. Routine tools-only PR67 was independently built/tested and merged under the latest authorization.
- Remaining queue after this: T-014, script half of T-041, T-044, T-046, T-047, T-007, BUG-003.

## Next (required before review)
1. Export item_template entry,class,bonding,Quality AS quality,startquest,flags to build/item-binding.tsv. Generate new dated world/character updates with tools/no-binding/generate.py; match configured protection IDs and reserve the next per-day codex SQL counters.
2. Back up/copy a real owned save using T-037 tooling. Run SQL validator and apply both updates twice to the owned copy; prove only eligible soulbound bits change and protected/unknown items plus unrelated flags remain identical. Do not apply main DB updates before human merges the core PR.
3. Start the newly built binary on Codex ports; test pickup, equip, protected quest/configured item, invalid-config fail-safe and config on/off/reload. GM .nobinding bind SLOT exercises the same flag path as transmog, but actual headless transmog still needs verification.
4. Add minimal headless hotfix/transmog support if needed. Existing CMSG_REQUEST_HOTFIX is in MiscHandler; item-sparse Bonding is byte offset 396 and Flags offset 8 in the serialized record (confirm from serializer). Hotfix table hash is 2442913102; hotfix_data PK is entry,type,hotfixDate. Capture actual wire values for enabled/protected/disabled states.
5. Run all three current regression scenarios on the new binary, update checklist/status, push and mark ready. Human reviews core changes; do not merge.

## Map additions
- game/Entities/Item/BindingRules and NoBinding; Custom/NoBinding; tests/no_binding; tools/no-binding.
