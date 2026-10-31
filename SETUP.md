# Setting up UltimateMoP for two agents

This folder holds the design doc, the agent coordination files and the game client
(`mop-clean/`). These steps turn it into the repo and give Claude Code and Codex their own
worktrees. (The repo already has a `README.md`, so this file is named SETUP.md to avoid a clash.)
Claude Code does these steps as task T-001; `HANDOFF.md` has the full sequence, including the
build and game data extraction.

## 1. Make this folder the repo

Fork https://github.com/1LineAtaTime/LoA-Pandaria-Playerbots on GitHub first. Then, in
`I:\UltimateMoP`:

**Before anything is added to git,** exclude the client and the extracted data. `mop-clean/` is 24 GB
of Blizzard data and must never be committed:

```
git init
mkdir .git\info -Force | Out-Null      # PowerShell; exists already after git init
Add-Content .git\info\exclude "mop-clean/`ngamedata/`n.worktrees/"
git config core.longpaths true
git remote add origin https://github.com/<you>/LoA-Pandaria-Playerbots.git
git remote add upstream https://github.com/1LineAtaTime/LoA-Pandaria-Playerbots.git
git fetch origin
git checkout -b playerbot --track origin/playerbot
```

The files here don't exist in the repo, so the checkout keeps them. Then add the worktree, client
and game data folders to `.gitignore` and commit the coordination files:

```
Add-Content .gitignore ".worktrees/`nmop-clean/`ngamedata/"      # PowerShell; in Git Bash: printf ".worktrees/\nmop-clean/\ngamedata/\n" >> .gitignore
git add .gitignore AGENTS.md CLAUDE.md STATUS.md TASKS.md SETUP.md HANDOFF.md docs status .claude   # after moving the settings file (step 4)
git commit -m "Docs: add design doc and agent coordination files"
git push origin playerbot
```

## 2. One worktree per agent

```
git worktree add .worktrees/claude -b claude/base playerbot
git worktree add .worktrees/codex -b codex/base playerbot
```

Agents create a task branch inside their worktree (`git switch -c claude/T-020-jewel-table playerbot`).

## 3. Per-agent databases and ports

Each agent needs its own four databases and ports (table in AGENTS.md). Until task T-002 builds
a script for it, create them by hand from the release dumps:

```
mysql -u<user> -p -e "CREATE DATABASE auth_claude; CREATE DATABASE characters_claude; CREATE DATABASE world_claude; CREATE DATABASE playerbots_claude;"
mysql -u<user> -p auth_claude < auth.sql
mysql -u<user> -p characters_claude < characters.sql
mysql -u<user> -p world_claude < world.sql
mysql -u<user> -p playerbots_claude < playerbots.sql
```

Repeat with `_codex`. In each agent's auth database, point the realmlist row at that agent's world
port. The game data folder (`dbc`, `maps`, `vmaps`, `mmaps`, `cameras`) is shared:
`I:\UltimateMoP\gamedata\`, extracted from `mop-clean/` during T-001.

## 4. Agent settings

- **Claude Code:** move `docs/claude-settings.json` to `.claude/settings.json` (create the `.claude` folder; it couldn't be written remotely). Edits are auto-accepted; git,
  builds, MySQL, Python and PRs on `claude/` branches run without prompts; force pushes, pushes to
  `playerbot`, hard resets, merges and `rm` ask first.
- **Codex:** copy the settings from `docs/codex-config.example.toml` into
  `%USERPROFILE%\.codex\config.toml`.

## 5. Running a session

```
cd I:\UltimateMoP\.worktrees\claude
claude            # then: "Take T-020."

cd I:\UltimateMoP\.worktrees\codex
codex --profile ultimatemop     # then: "Take T-010."
```

Each agent reads STATUS.md, TASKS.md and docs/MAP.md, works in its own worktree, and opens a PR
with a status fragment.

## 6. Your routine

1. Review and merge PRs (ask the other agent to review first if you like).
2. Fold the fragments in `status/` into `STATUS.md`, add useful paths to `docs/MAP.md`, delete the
   fragments, commit.
3. Answer anything marked `blocked:` in TASKS.md.
4. When a milestone is done, update the milestone in STATUS.md and add the next tasks to TASKS.md
   from `docs/DESIGN.md`.
