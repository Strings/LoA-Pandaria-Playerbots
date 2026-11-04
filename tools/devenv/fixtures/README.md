# Checkpoint fixtures and bot baselines (T-007)

Standard-library Python tool for a named agent worktree and its private generated test runtime. It reuses T-048 checkpoints to level, teach skills, equip and position characters; it never writes raw character/item preset SQL. The five shipped Orc warrior fixtures are Ckone (1), Ckthirty (30), Cksixty (60), Ckeightyfive (85), Ckninety (90). Level 1 keeps its native starting outfit; higher checkpoints use the existing green/blue/epic gear factory. Bags, riding and faction-capital teleport also reuse existing steps.

```
python tools/devenv/fixtures/fixtures.py --runtime build/devenv --mysql-bin "C:/Program Files/MySQL/MySQL Server 8.0/bin" provision
python tools/devenv/fixtures/fixtures.py --runtime build/devenv --mysql-bin "C:/Program Files/MySQL/MySQL Server 8.0/bin" verify
```

Start auth/world on your own ports first. Connection credentials come from that runtime's config; MOP_MYSQL_BIN can supply the client directory. The tool checks metadata, ports and all database connections against the named worktree and allows only localhost. A worktree lock prevents overlapping fixture operations. It rejects online fixtures and names owned by another account/race/class, and preflights the full roster before checkpoint changes.

A dedicated CHECKPOINT_<AGENT> account is created with random credentials stored only in the ignored runtime's checkpoint-fixture-account.json. It has administrator commands solely so the self-applied headless checkpoints can invoke the existing gear factory. All fixture characters log out with GM mode off. This is a test account, not a normal-player permission fixture. Existing account credentials are never reset, and an account with unmanaged characters is rejected.

The non-secret fixture-manifest.json maps stable IDs (warrior-001/030/060/085/090) to local GUIDs, actual equipment entries/GUIDs and checkpoint names. Repeating provision verifies the manifest and preserves the exact existing items. A changed fixture requires an explicit provision --reset (checkpoint reapply, upgrades only) or the exact baseline restore below. Failed partial provisioning never writes a complete manifest; rerun to resume.

## Pre-generated bots and cold-file reset

Create the small offline bot pool once using the bootstrap profile (no existing roster is needed):

```
python tools/devenv/setup.py --agent codex --gamedata I:/UltimateMoP/gamedata --binaries build/bin --output build/bot-bootstrap --profile test --load bootstrap
```

Stop any world using your ports, then start the bootstrap world. It enables the native factory with three accounts and disables automatic bot login (both population limits are zero). This profile is distinct from none/local and cannot run the regression suite or fixture provisioning. Account count and login switches live in tools/devenv/profiles.json. After it creates the characters, collect a 20..50-name roster from your own databases and stop the bootstrap world. Generate a test/local runtime with --roster to explicitly initialize/anchor that roster using tools/devenv/stage_local.py, then .botscenario <name> randomize <level> once. Stop cleanly after initialization; use test/none for ordinary fixture tests. All runtimes must use the same private databases and bot-account prefix. A repeated bootstrap reuses existing accounts; normal test runs restore the saved baseline instead of re-randomizing it.

Copy tools/backup/config.example.json into build/fixture-backup.json. Keep all four databases paired, including auth (bot accounts/credentials), characters (gear/characters), world (checkpoint version) and playerbots (AI state). Use an unused capture port and worktree-local snapshot/check roots. Stop your private world before capture; fixture/bot rows must be offline. Select 20..50 existing bot characters. The tool verifies the five fixtures, records the bot roster, and delegates capture to T-037:

```
python tools/devenv/fixtures/fixtures.py --runtime build/devenv --mysql-bin "C:/Program Files/MySQL/MySQL Server 8.0/bin" snapshot --config build/fixture-backup.json --bot-count 20
python tools/devenv/fixtures/fixtures.py --runtime build/devenv --mysql-bin "C:/Program Files/MySQL/MySQL Server 8.0/bin" restore build/fixture-snapshots/SNAPSHOT build/fixture-restored/data
```

Snapshots stay gitignored. Keep the runtime's private checkpoint-fixture-account.json and connection configuration alongside your private backup; the non-secret baseline manifest deliberately contains no login password. fixture-baseline.json records their stable fixture map and pre-generated bot roster. T-037 imports a consistent capture into a new isolated MySQL instance, stops it, hashes the cold files and restores by verified file copy. The restore destination must be new and below this worktree's build directory; existing databases/services are never replaced. Use the matching MySQL version on a separate unused local port. See tools/backup/README.md for launching restored data. Restoring into the shared service is outside this tool.

The test-only idempotent SQL is sql/fixtures/2026_11_04_codex_00_checkpoint_fixtures.sql. provision applies only its fixture-* template rows to the private world database and reloads checkpoints through the first fixture. No new schema, core hooks, config timers or production update are introduced. Limits live in sql/fixtures/characters.json.

Tests: python -m unittest discover -s tools/devenv/fixtures -v. Do not regenerate bots or regear all characters on ordinary test runs: verify the manifest or restore its baseline.
