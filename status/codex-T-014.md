# T-014: All professions and extra profession bar

**Agent:** codex
**Branch:** codex/T-014-profession-bar
**Task status:** in progress

## What changed
- Confirmed existing MaxPrimaryTradeSkill core key and raised the shipped value to 11.
- Implementing a small profession inspection/learning adapter and an addon bar for known professions beyond the client's first two. No new durations.

## How it was tested
- Audited core slot initialization/decrement/removal and verified all eight rank spell IDs for each primary profession against spell_ranks.
- Implementation, unit/addon tests, default build and live checks remain.

## Next
- Finish module/addon, push before build, then test all eleven slots, rank handling and combat-safe addon updates. Claude approval required before a routine merge.
