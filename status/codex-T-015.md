# T-015: Configurable no-binding gear

**Agent:** codex
**Branch:** codex/T-015-no-binding
**Task status:** review

## What changed
- Config-gated ordinary item binding policy, preserving quest/starter/artifact items, configured entries and registered per-instance progression checks.
- Minimal core call sites cover pickup/equip/transmog flag writes, loaded items, binding enchants and item-sparse tooltip flags/bonding. No widely included header changes.
- Config reload refreshes online items and hotfix versions. Catalog metadata is batched within packet limits. GM status/item/bind/pickup/refresh commands and brief login intro.
- Deterministic numeric-catalog cleanup generator with explicit protected entry/GUID inputs; original template bonding remains intact.

## How it was tested
- Default RelWithDebInfo worldserver build: passes, no new warnings in touched files. All 15 current CTest suites pass; six generator tests and Python syntax checks pass.
- Both generated SQL files: validator reports zero issues. Catalog has 80,064 templates, 71,870 eligible.
- Real Codex save copied to disposable characters/world DBs. All 151 item rows, including five sentinel kinds, match the expected soulbound-bit-only delta; explicit entry/GUID protection works; other character tables hash identically. Both SQL updates applied twice with identical results, then applied to Codex DBs only. Original dump retained under ignored build/.
- Live normal pickup (not .additem's GM-unbinding shortcut): ordinary BoP 13953 unbound/tradeable, quest 2794 bound. Equip 18832 and actual headless transmog to 2244 succeeded; persisted transmogrifyId=2244 and source flags remain unbound.
- Actual item-sparse packets: ordinary binding=0, quest binding=1; disabling restores normal BoP pickup and binding=1 tooltip. Configured 13953 protection stays bound; ordinary items created while disabled clear on enabled relog. Invalid protected IDs disable safely; defaults restored.
- All current regression scenarios pass against the final server: capture-idea, custom-reload-rng, login.

## Decisions made
- NoBinding.Enable=1, NoBinding.ProtectedItemIds="", NoBinding.IntroOnLogin=1. No durations added. Artifact quality conservatively stays protected.
- Future progression/storage providers register protection before player loading and export protected low GUIDs before generating an explicit cleanup. Unknown templates stay untouched.
- Disabling restores future binding/tooltip behavior, not historical flags removed by the explicit cleanup.

## For the other agent
- Core changes require review. Draft PR 69 is ready; no self-merge without required approval.
- Updates: world/2026_11_01_codex_00_no_binding_hotfix.sql; characters/2026_11_01_codex_01_no_binding_cleanup.sql. Codex applied both; main DBs must receive them after merge, with backups before the sizable cleanup.
- Reproduce: .nobinding pickup ENTRY; .nobinding item SLOT; .nobinding bind SLOT; .nobinding refresh. Headless item-hotfix ENTRY inspects actual wire data; transmog NPC_GUID SLOT SOURCE_ENTRY SOURCE_GUID exercises the client packet. GM .additem deliberately unbinds its additions, so use pickup for binding tests.
- Latest-base integration on playerbot a273234b also passes the build, all 15 unit suites, actual tooltip/loaded-item inspection and all three regressions. Already-merged loadout/storage prerequisites were applied only to Codex DBs.
- Codex runtime currently runs worldserver-T015-integrated.exe on its own ports/DBs, no random bots. Main fixture guid 1 is back at the quiet staging point with GM on.

## Next
- Claude/human core review. After approved merge, back up and apply only these two SQL updates to main DBs as required by AGENTS.md.

## Map additions
- game/Entities/Item/BindingRules and NoBinding; Custom/NoBinding; tests/no_binding; tools/no-binding; headless item-hotfix/transmog verbs.

## Review follow-up
- Claude approved with nonblocking notes. Rebased on playerbot 2c654e81; preserved merged profession registration/settings/task state. Owned no-binding policy/code/tests unchanged; existing build/live evidence above precedes this rebase. Human core merge held. No main SQL applied: the large characters cleanup requires human approval, already raised by Claude.
