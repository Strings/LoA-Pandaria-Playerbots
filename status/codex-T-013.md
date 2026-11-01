# T-013: Persistent raid retention and configurable resets

**Agent:** codex
**Branch:** codex/T-013-raid-resets
**Task status:** in progress

## What changed
- Config-gated raid retention, a two-day configurable reset period, solo-owner/raid-leader abandonment and GM inspection/force-reset/force-abandon.
- Existing resettime field carries partial/finished metadata; synchronous progress writes preserve it across restart. No schema migration or new table.
- Narrow fixes: GroupMgr restores all group binds under MySQL strict grouping; Terrace restores its legacy array into core boss states and keeps its saved string; Creature holds defeated raid bosses until reset. Cleared raid encounters cannot downgrade themselves while retention is enabled.

## How it was tested
- Rebased on playerbot 5f3b23a7 (documentation-only advance). Default RelWithDebInfo build passes with no new warnings in touched files. All 12 unit suites pass.
- Live 20-second test cadence: canonical partial fixtures survive startup/empty cycles; expired finished/empty saves clear; first boss in progress defers a boundary; script progress, group binds and respawns survive restart.
- Live completion clears all binds/respawns; occupied reset releases IDs only after map destruction and the ID is reused. Non-owner denied; solo owner and raid leader can abandon with an offline human and bot bound. Enable/notification toggles pass.
- custom-reload-rng and login regressions pass. Actual Protectors deaths wrote three held respawn records; fresh-instance repeat and Capture regression remain.

## Decisions made
- Human approved expanded retention/abandonment scope and confirmed no existing raid saves at rollout. No migration.
- The period is read from config each cycle. Boss death retention is a held state until reset, not another duration.

## For the other agent
- Core changes: human review required; do not merge. No Heartseed edits.
- Test setup applied already-merged T-039 GM-tool SQL only to Codex databases. No T-013 SQL to apply.

## Next
- Complete fresh-instance real-death check and capture-idea regression; mark ready and review.

## Map additions
- game/Instances/RaidRetention and RaidResetPolicy; Custom/RaidRetention; tests/raid_retention.
