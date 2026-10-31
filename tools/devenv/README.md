# Per-agent development environment

Python 3.9+ standard library. No build, GUI, system settings or global MySQL
service changes. Run from the worktree root; all generated files stay in that
worktree's ignored build directory. Set MOP_DB_PASSWORD before generating configs.

```
python tools/devenv/setup.py --agent codex --gamedata I:/UltimateMoP/gamedata --binaries I:/UltimateMoP/build/bin/RelWithDebInfo --output build/devenv --profile test --load none
```

Agent ports: Codex 3744/8105/7898; Claude 3734/8095/7888; Gemini 3754/8115/7908 (auth/world/SOAP).
Database suffix matches the agent. The generator copies the two existing server
executables and libmysql, libcrypto, libssl and **legacy.dll**. OpenSSL's provider
search path is the executable directory; without legacy.dll the first world
connection can abort even though SOAP works. No compiler flags or builds change.

Start with `build/devenv/start.ps1 -Server auth` and `-Server world`; the launcher
uses an explicit absolute -c config path, the runtime as working directory, and
hidden server windows. playerbots.conf is read from the working directory.
Stop world via SOAP `server shutdown 1`, then stop only the auth process you
started. Never stop all processes by name or the shared database service.

## Database initialization

Add --mysql-bin PATH (or MOP_MYSQL_BIN), --initialize, and four arguments
`--dump auth=PATH --dump characters=PATH --dump world=PATH --dump playerbots=PATH`.
Use database-local plain SQL v1.0.4 release dumps (no USE/CREATE/DROP DATABASE).
Existing populated agent databases are skipped, never overwritten. Failed imports
remain for diagnosis; existing tables are not cleared automatically. Re-run on a
new empty set or repair your own partial import deliberately.

`--configure-db` updates only the chosen agent's realm id 1 port. If MOP_GM_USER
and MOP_GM_PASSWORD are supplied, it creates the named account if missing and
ensures administrator access. It does not reset an existing password. Do not
point a setup run at another agent's databases without authorization. DB host,
port, user and password environment variable are configurable. Passwords never
appear in command arguments or generated metadata, but do appear in the private
server configs as required by the core; keep build/ out of version control.

## Dev/test and load profiles

profiles.json is the editable policy: all shortened timer values live there.
Optional profile.d/*.json files overlay dev/test world and bots values, in filename
order; {repo} expands to the absolute worktree path. This lets independent system
PRs add profile settings without duplicating the base profiles.
Both profiles unload idle grids and disable transport grid loading. No general
"preload every grid" key exists in this fork; no unsupported key is invented.
Test shortens session admission, player save, weather and grid cleanup timers;
logs are reduced to server INFO/root WARN, with playerbots DEBUG for probes.
The normal server log overwrites at startup; this is not a size-rotation service.
Future custom systems add their timer overrides here when their config keys land.

- none: bots disabled, no automatic login. Default for build-free checks.
- local: explicit roster of 20..50 existing random bots, automatic population and
  level teleports off. Supply --roster with one name per line, adjust the local
  anchor for their faction/zone in profiles.json, then start the server and run
  `python tools/devenv/stage_local.py build/devenv`. It logs in each named bot and
  starts a bounded wander scenario at that one anchor. Default anchor is in
  Stormwind: use Alliance bots or change it. Scenarios expire after the configured
  duration; re-stage for another run. This is deliberate local staging, not a
  claim that RandomBotMaps restricts bots to a zone. Bot fixtures/snapshots are
  T-007; initialization may create the local account pool on first start.
- scale: 500..2000 bots, requires --performance. Use only for performance work.

The generator does not select LLM providers: none exist yet. New provider keys
must explicitly choose scripted in test policy when introduced.

Tests: `python -m unittest discover -s tools/devenv -v`.
