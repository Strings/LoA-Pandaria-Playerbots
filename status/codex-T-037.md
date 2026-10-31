# T-037: Save snapshots and migration preservation

**Agent:** codex
**Branch:** codex/T-037-save-safety
**Task status:** review

## What changed
- tools/backup: nightly worker, versioned SHA256/row-count manifests, marked snapshot rotation and file-copy restore into new destinations only.
- All source tables are read-locked during one dump (the real characters save has four MyISAM tables). Capture imports into a fresh private MySQL instance, closes it, and retains its cold data files. The shared MySQL80 service is never stopped or written by snapshots.
- Custom-table migration check clones a real save, applies allowlisted SQL twice with optional sentinel rows, checks idempotence and preservation of all core tables, and drops only its newly created scratch database.
- All paths, ports, credentials, intervals and retention are config/environment driven. No system schedule is installed: nightly is an opt-in persistent worker.

## How it was tested
- Two unit tests: migration boundary/unsafe SQL rejection and file integrity/new-destination restore safeguards.
- Actual snapshot of four private fixture databases; cold files copied to a new datadir, booted on private port 3345, and sentinel rows verified in every database. Capture used private port 3344.
- Actual pending T-020 SQL read from Claude's worktree (not edited): cloned characters_codex; applied migration, seeded rank/Growth/loadout/essence/story rows, applied again. Both dumps identical; 132 core tables unchanged. Migration SHA256 f9e790235cdb48195b5e9ed1e1243bb07b718c760f46fa16fa2dfe3d9d0f59a1.
- Real-save report: build/migration-checks/characters_codex_check_355aad91761f/report.json (ignored).
- No full build, GUI, shared service stop or existing database rewrite.

## For the other agent
- T-020 migration preservation passes. README has the command and --seed workflow; no merge dependency on T-020 code.
- Capture blocks source writes while dumping. Quiesce DDL and choose a quiet backup window. Restore needs the same MySQL version/platform and a separate instance; activating it in the shared service remains ask-first.
- Snapshot integration was a four-database fixture, not a full world-size nightly soak. The migration test used the actual character save.

## Map additions
- tools/backup/save_safety.py, config.example.json, README.md.
