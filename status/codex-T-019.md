# T-019: Solo configuration defaults

**Agent:** codex
**Branch:** codex/T-019-solo-config
**Task status:** review

## What changed
- Rebased onto current playerbot; recovered SoloConfig work, dropped T-038 leftovers. No Heartseed edits.
- Shipped/profile defaults enable existing cross-faction and taxi hooks, guaranteed profession gains, lighter death penalties, and uncapped weekly currency.
- Added cached/reloadable fishing and weekly-cap switches, GM probes and per-key headless smoke coverage.
- Guarded cap re-enabling above the weekly threshold so rewards cannot subtract/underflow currency. Holding caps, accounting and the Valor bonus threshold remain.

## How it was tested
- Release worldserver build; no warnings in touched files (existing PathGenerator C4715 warnings remain).
- tools/devenv/test_solo.py: 2 pass; developer-environment tests: 6 pass; CTest: 6 suites pass, including currency boundaries.
- Private Codex test/none runtime: all 28 effective keys, login taxi access and reclaim delays; fishing gain/rank cap and legacy chance; uncapped Valor/arena/rated-BG currency paths, holding cap, and cap re-enable above limits; login regression.
- Live evidence: build/T019-final-*.log and build/regression/T019*. Earlier alchemy craft check: 70/75 to 71/75.

## Decisions made
- No provisional features. Retained missing-key compatibility for the two new switches.

## For the other agent
- Claude reviews; leave PR open. No new SQL or shared runtime changes. Backed up characters_codex and applied the merged T-049 account migration required by the rebased core.
- Social keys were checked as effective loaded values, not a full multi-account social regression.
- Spirit-healer durability remains 25%; PvP reclaim retains 30-second base delay; config docs spell out both limits.

## Next
- Claude review, then human merge.

## Map additions
- World/SoloConfig caches switches; World/CurrencyLimits.h owns tested cap arithmetic.
- Custom/SoloConfig contains GM probes; tools/devenv/solo_smoke.json is the per-key live scenario.
