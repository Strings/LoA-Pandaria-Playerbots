# Handoff: T-075 pre-playtest smoke run (Gemini Flash)

**From:** Claude Code, 2026-11-04. **To:** Gemini Flash in Antigravity, worktree `I:\UltimateMoP\.worktrees\gemini`.

The human pastes this prompt into Antigravity: "Read `docs/agents/handoffs/gemini-T-075.md` and do it exactly."

This is a report-only task. Fix nothing, edit nothing outside `docs/agents/smoke-run.md`. Last time the run was
never pushed, so step 8 is the one that matters most.

## Steps

1. In `I:\UltimateMoP\.worktrees\gemini`, run `git fetch origin`, then
   `git switch -c gemini/T-075-smoke-run origin/playerbot`. Write down the commit: `git rev-parse --short HEAD`.
2. Don't build: GEMINI.md says no full server builds. Use Claude Code's binaries, built from the same
   `playerbot` commit:
   `I:\UltimateMoP\.worktrees\claude\build\bin`. Only read from that folder, never write to it.
3. Regenerate your runtime (set `MOP_DB_PASSWORD` first, from your existing `build/devenv` confs):
   `python tools/devenv/setup.py --agent gemini --gamedata I:/UltimateMoP/gamedata --binaries I:/UltimateMoP/.worktrees/claude/build/bin --output build/devenv --profile test --load none --db-user ultimatemop`
4. Start `build/devenv/authserver.exe` and `build/devenv/worldserver.exe -c <absolute path to build/devenv/worldserver.conf>`.
   Wait for the world server's "ready" line. Ports must be 3754 / 8115 / 7908. If any other port shows, stop and
   report it.
5. Set `MOP_ACCOUNT`, `MOP_PASSWORD`, `MOP_SOAP_USER`, `MOP_SOAP_PASSWORD` for your fixture account (see
   `contrib/headless-client/README.md`), then run the full suite once:
   `python tests/regression/run.py --runtime build/devenv`
6. Over SOAP or the headless client, run `.checkpoint list` and `.spawnthin`, and save each reply.
7. Write `docs/agents/smoke-run.md`:
   - The `playerbot` commit from step 1, and the date.
   - A table with one row per scenario file in `tests/regression/scenarios/`: name, pass or fail, and for a failure
     the failing step and the server's reply (at most 3 lines).
   - The `.checkpoint list` and `.spawnthin` replies (first 10 lines each).
   - Paste in the final summary line of `run.py`, not the whole log.
8. `git add docs/agents/smoke-run.md`, commit `Docs: T-075 smoke run results`, `git push -u origin HEAD`, and open a
   PR against `playerbot`:
   `gh pr create --repo Strings/LoA-Pandaria-Playerbots --base playerbot --fill`
   Put the `run.py` summary line in the PR body. **Do not merge.**
9. Stop your servers, then set T-075's status in `TASKS.md` to `review` in the same PR.

## Done means

- PR open from `gemini/T-075-smoke-run`, containing only `docs/agents/smoke-run.md` and the TASKS.md status line.
- Every scenario in the folder has a row (27 when this was written).
- If a step fails twice, write what happened in `smoke-run.md`, push anyway, and say so in the PR.
