# T-013: Persistent raid retention and configurable resets

**Agent:** codex
**Branch:** codex/T-013-raid-resets
**Task status:** in progress

## Approved scope
- Preserve partially cleared raids across empty periods and restarts.
- Finished raids reset on a configurable two-day period.
- Explicit abandonment belongs to the raid leader, or the solo owner of the bind. Bots never block abandonment.
- Add GM bind listing, force-abandon and force-reset commands.
- Keep core behavior behind a config key and as small as possible. The human confirms there are no existing raid saves on any database; no migration is required.

## Audit
Startup cleanup and global resets currently delete saves/binds by map/difficulty without progress checks. Both paths must honor persistent per-instance retention. No timer-only partial implementation will ship.

## Implementation in progress
Raid-only startup/cycle retention, explicit owner/leader abandonment and GM probes are implemented. Atomic script/credit saves reuse existing resettime metadata (reserved UINT32_MAX means partial; finite timestamp means finished), leaving encounter bits and raid scripts unchanged. MoP world encounter rows are missing, so existing script BossState and validated LFG wing masks cover those raids; normal Throne excludes heroic-only Ra-den. No schema changes.

Eleven unit suites pass on current playerbot, including boundaries, permissions, wing completion and script masks. The default RelWithDebInfo build is running; live save/restart/abandon checks remain.

## Next
Finish build and live verification. PR #45 stays draft until build and tests pass; Claude reviews, do not merge.
