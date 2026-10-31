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

## Current state (2026-10-31, 18:10 Eastern)

- **Claude:** T-038 finished from Codex's branch and merged (reload registry plus seeded test RNG). Design notes
  added for bot behavior: PvP sparring (T-059, parked), telemetry context and offline methods, social coupling,
  transition seams (all under DESIGN.md, Human-like bots and PvP coordination).
- **Codex:** out of usage for this window. T-019 (solo config) is pushed on `codex/T-019-solo-config` (draft
  PR 10) and needs a rebase; its T-038 worktree still holds the uncommitted SoloConfig files. Run it at medium.
- **Gemini:** T-030, T-050 and T-033 merged. Next: the Flash queue (T-051 to T-058) once it is on Flash.
  Follow-up worth queueing: `tools/twink-tuning` should read DBCs through `tools/dbc`.
- **Open, unassigned:** T-060 (lld-link), T-031 (balance sim).
- **Main databases:** all agent SQL applied through `2026_10_31_gemini_00` (backup in
  `build/db-backup-2026-10-31-pre-agent-sql/`).

## Known issues

- Once, not reproduced in four tries: the world thread hung in `Player::~Player` (deque teardown) under
  `HandlePlayerLoginOpcode` -> `LogoutPlayer` when a master relogged while its bot alt was grouped with it.
  Core or playerbots, not Heartseed. Dump in `.worktrees/claude/build/bin/Crashes/`.
- The GMSuite addon (`addons/GMSuite/`) has not been tried in a real client yet; that's for the human playtest.
