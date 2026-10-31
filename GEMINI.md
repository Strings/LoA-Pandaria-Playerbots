@AGENTS.md

## Gemini specifics

- Read `AGENTS.md` first; it is the source of truth. This file only adds what is specific to you.
- Your worktree is `.worktrees/gemini`, your branch prefix `gemini/`, your ports and databases are the Gemini
  row in AGENTS.md (`python tools/devenv/setup.py --agent gemini ...` sets them up).
- Claude Code orchestrates: take your tasks from `TASKS.md` (owner `gemini`) and ask Claude Code, through the
  human, before picking up anything else. Claude Code reviews your PRs before they merge.
- Until told otherwise, do tooling and unit-tested work only: no full server builds (they compete with the
  other agents' builds for the machine).
- Do not use computer-use or browser tools without asking first.
