# Save safety

Python 3.9+ and MySQL 8 client/server binaries, no Python dependencies. Commands
below run from the worktree root. Copy config.example.json to an ignored path
under build/, set MOP_DB_PASSWORD, and choose **your own** database names and
unused capture port. No credentials belong in the config. Never use another
agent's databases without authorization.

## Nightly snapshots and file-copy restore

```
python tools/backup/save_safety.py --config build/backup.json snapshot
python tools/backup/save_safety.py --config build/backup.json nightly
python tools/backup/save_safety.py --config build/backup.json restore SNAPSHOT build/restored/data
```

The nightly worker captures immediately, then waits snapshot_interval_seconds
(86400 by default). Run it in a persistent terminal/background process; this
does not install a system scheduled task or keep running after its host process
is stopped. An error exits visibly instead of silently claiming a backup.

Capture takes read locks across all configured base tables, then uses one
mysqldump of all configured databases,
imports into a newly initialized disposable localhost-only MySQL instance, then
shuts that instance down cleanly. The completed snapshot contains cold InnoDB
files, row counts, server version and SHA256 hashes. Restore verifies hashes
and copies files into a **new** destination. Existing saves/services are never
overwritten or stopped. Retention removes only marked snapshots under the
configured worktree-local snapshot root. Failed captures retain pending files
for diagnosis and never replace the last good snapshot.

This avoids unsafe copying of live InnoDB files or stopping the shared MySQL80
service. Initial capture/import is slower than a physical hot-backup product;
restore is file-copy, with no SQL replay. Use the same MySQL version/platform to
start restored data on a separate port; do not copy individual .ibd files into
an existing instance. Moving a restored save into the shared service needs the
human's approval. Test restoring periodically and keep an independent off-machine
copy for hardware failure. Database DDL must be quiescent during capture.
Writes wait for the capture's read locks (including this fork's MyISAM tables);
use a quiet backup window. source_lock_timeout_seconds controls lock acquisition.
Passwordless root exists only in the disposable bound-
localhost instance; do not expose its capture port externally.

## Migration check against a real save

```
python tools/backup/save_safety.py --config build/backup.json check --source characters_codex --table heartseed_state --table heartseed_loadout --table heartseed_essence --table heartseed_story_flag --seed build/heartseed-sentinel.sql sql/updates/characters/2026_10_31_claude_00_heartseed_state.sql
```

The source is read only. A unique scratch database receives a real dump, then
the migration. Optional seed SQL inserts representative custom-system save rows
before reapplication. Applying twice must produce identical data/schema dumps;
core tables must also be identical before/after. A JSON report records hashes.
Only the newly created scratch database is dropped; diagnostic dumps stay in
the ignored check directory. Dump/check rights and CREATE/DROP DATABASE on the
source-name prefix are required (the imported development user has them).

The strict preflight accepts CREATE TABLE IF NOT EXISTS, ALTER TABLE and simple
INSERT/UPDATE/DELETE on explicit custom table names. Cross-database names,
client commands, executable comments, routines, nested SELECT and multi-table
writes are rejected. More complex migrations need a separately reviewed adapter;
do not loosen this guard just to pass a migration. This checks preservation and
idempotence, not semantic correctness of a schema refactor. Add sentinel rows
for every new system and test meaningful old-save edge cases.

Tests: `python -m unittest discover -s tools/backup -v`.
