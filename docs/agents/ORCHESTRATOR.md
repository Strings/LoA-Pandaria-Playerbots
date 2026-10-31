# Orchestrator handoff

Living note for whichever Claude Code session is coordinating the agents: the main subscription or the
backup one that takes over late in the week. Read it at session start with AGENTS.md and TASKS.md; update
it before you stop. It holds current state; standing rules are in AGENTS.md.

## Weekly rhythm

1. The week starts with Claude Code (Opus, medium), Codex (Sol, medium) and Gemini (Argon) all running.
2. When Argon's weekly quota runs out, Gemini switches to Flash and works through the Flash queue in TASKS.md,
   so Claude and Codex keep the harder work.
3. When the main Claude subscription runs out, the backup Opus subscription continues from this note.

## Standing practice

- Claude Code merges routine PRs itself; ask the human before a major merge (AGENTS.md).
- After a merge with SQL, apply its updates to the main databases (back up first if sizable; `.sql` only).
- Gemini and Flash PRs merge only after Claude's review. Codex PRs: review when asked; Codex has been slow
  and thorough, so give it small, sharply scoped tasks away from files others are changing.

## Current state (2026-10-31, evening)

- **Claude:** T-039 (GM tools, first set) in progress on `claude/T-039-gm-tools`.
- **Codex:** back at Sol medium. Finishing T-019 (solo config): rebase `codex/T-019-solo-config` onto `playerbot`,
  fold in the SoloConfig files left uncommitted in its T-038 worktree, open the PR. Then T-010 (mount speed).
- **Gemini:** one last Argon task, T-040 (`/idea` and `/bug` capture). Then the Flash queue (T-051 to T-058).
  Follow-up worth queueing: `tools/twink-tuning` should read DBCs through `tools/dbc`.
- **Open, unassigned:** T-060 (lld-link), T-031 (balance sim).
- **Main databases:** all agent SQL applied through `2026_10_31_gemini_00` (backup in
  `build/db-backup-2026-10-31-pre-agent-sql/`).

## Known issues

- Once, not reproduced in four tries: the world thread hung in `Player::~Player` (deque teardown) under
  `HandlePlayerLoginOpcode` -> `LogoutPlayer` when a master relogged while its bot alt was grouped with it.
  Core or playerbots, not Heartseed. Dump in `.worktrees/claude/build/bin/Crashes/`.
- The GMSuite addon (`addons/GMSuite/`) has not been tried in a real client yet; that's for the human playtest.
