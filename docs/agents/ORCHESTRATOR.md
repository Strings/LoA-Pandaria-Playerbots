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

## Current state (2026-11-01, early morning)

- **Claude:** T-039 merged (PR 48: `.clock`, `.bot why`, `.scenario`, `.spectate`; SQL applied to the main
  databases). Merged Codex's T-010 (43), T-019 (46) and T-016 (49). Builds now queue machine-wide (PR 44).
  Taking over T-040 (`/idea` and `/bug` capture) from Gemini once its work in progress is pushed to
  `gemini/T-040-capture`.
- **Codex:** Sol medium, doing well at that setting. T-013 (raid resets) in progress on PR 45; the human approved
  the larger scope (keep unfinished raids, raid-leader-only abandon, no existing raid saves to migrate). Then T-011.
- **Gemini:** out of Argon for the week. Flash works the Flash queue (T-051 to T-058) on request.
- **Open, unassigned:** T-060 (faster builds, see the task: sccache caches nothing with PCH on), T-031.
- **Main databases:** all agent SQL applied through `2026_10_31_claude_19` (T-039).

## Known issues

- Once, not reproduced in four tries: the world thread hung in `Player::~Player` (deque teardown) under
  `HandlePlayerLoginOpcode` -> `LogoutPlayer` when a master relogged while its bot alt was grouped with it.
  Core or playerbots, not Heartseed. Dump in `.worktrees/claude/build/bin/Crashes/`.
- The GMSuite addon (`addons/GMSuite/`) has not been tried in a real client yet; that's for the human playtest.
