BLOCKED: weekly currency caps and guaranteed fishing have no working config switches; authorize core hooks after build tooling, or defer those portions?

# T-019: Verified solo config defaults

**Agent:** codex
**Branch:** codex/T-019-solo-config
**Task status:** blocked

## What changed
- 26 supported config values in worldserver.conf.dist and an independent dev/test profile overlay: cross-faction hooks, all taxi paths, 100% crafting/configured gathering rolls, prospecting/milling, ordinary death durability off, sickness disabled at MoP cap, reclaim escalation off.
- Standalone private-runtime apply helper and source-backed SOLO_CONFIG.md audit. No C++ changes.
- Weekly caps unchanged: the legacy Conquest key rejects 0 and its loaded enum has no consumers; GetCurrencyWeekCap uses DBC/rating caps. Fishing chance above 75 is hardcoded separately. Those require core hooks and compiled validation; T-019 is not complete.

## How it was tested
- Two unit tests verify every key is actually loaded, shipped/profile defaults agree, and applying twice preserves bytes.
- Live Codex SOAP reload config succeeded. Headless GM fixture learned alchemy, set skill 70/75 and crafted spell 2330; core log showed Chance 100.0% taken, and saved character_skills value became 71/75.
- Character is the disposable Codextwink fixture. No main/Claude DB/config changes, GUI or full build.

## For the other agent
- Draft PR contains reviewable config-only work; cap/fishing extension awaits the human's scope decision and build tooling.
- PvP base reclaim delay and spirit healer's hardcoded 25% durability charge remain; disabled escalation and ordinary-death loss are accurately documented.
- T-002 PR #4 consumes the optional overlay once merged. Standalone apply helper works without stacking its code.
