# T-019: Solo config defaults and authorized core hooks

**Agent:** codex
**Branch:** codex/T-019-solo-config
**Task status:** review (draft; compiled validation deferred by the human)

## What changed
- 28 config values in worldserver.conf.dist and independent dev/test overlay: cross-faction hooks, all taxi paths, guaranteed crafting/gathering/fishing, ordinary death durability off, sickness disabled at MoP cap, reclaim escalation off, no weekly currency caps.
- Human authorized core hooks on 2026-10-31 and explicitly deferred compiled validation until Claude's build tooling lands.
- New cached bools Currency.WeeklyCaps.Enable (shipped 0; omitted fallback 1) and SkillChance.Fishing.Always (shipped 1; omitted fallback 0). Config reload supported.
- Weekly-cap getter returns zero/unlimited; both Conquest meta award clamps respect that sentinel. Total holding limits and weekly/season counters/reset remain. Valor of the Ancients uses the original DBC bonus milestone independently of earning limits.
- Fishing selects 100% through the existing skill-up path, preserving learned caps/gain amounts; disabling restores the original formula.

## How it was tested
- Two Python checks pass: all keys loaded, shipped/profile defaults agree, runtime application byte-idempotent. git diff --check passes.
- Prior live test of unchanged alchemy config path: fixture skill 70/75, craft 2330, logged Chance 100.0% taken, saved 71/75.
- New C++ hooks are NOT compiled or runtime-tested. No full build or GUI started. Detailed post-build currency/PvP/fishing/toggle test plan in tools/devenv/SOLO_CONFIG.md.

## For the other agent
- Keep PR #10 draft pending compiled validation. Task scope approval is resolved; there is no remaining permission question.
- PvP base reclaim delay and spirit healer's separate 25% durability charge remain documented.
- T-002 PR #4 consumes the overlay once merged. Existing binaries ignore the two new keys; deploying config alone does not activate these hooks.
- No main/Claude environment changes; STATUS.md unchanged.
