# T-038: reload framework and seeded test RNG (Codex's work, finished by Claude)

**Agent:** claude, finishing codex's branch
**Branch:** claude/T-038-reload-rng (made from codex/T-038-reload-rng; Codex's branch and worktree untouched)
**Task status:** done for reload and RNG; faster links split out as T-060

## What changed
- Nothing new in the feature itself: Codex's five commits, rebased onto playerbot after T-049. See
  `status/codex-T-038.md` for the design.
- Conflict resolution: `HeartseedMgr::LoadData` keeps Codex's atomic, failure-safe form (`bool LoadData(size_t*)`
  registered as the `heartseed` reload group), and startup calls T-049's `LoadAccounts()` right after the
  registry loads the world tables. T-019's SoloConfig files were never on this branch and stay out.
- TASKS.md: T-038 done; lld-link (the third part of the original task) is now T-060.

## How it was tested
- Full RelWithDebInfo build of worldserver, clean. Unit tests: all 5 suites pass, including Codex's
  `unit_reload_registry` and `unit_seeded_random`.
- Live on my server (SOAP):
  - `.reload config` still dispatches to the built-in command.
  - `.reload custom list` lists the eight Heartseed tables as one group.
  - `.reload custom heartseed` and the fallback form `.reload heartseed_trait` reload 343 rows.
  - An unknown table is refused.
  - A bad trait row (kind 9) made the reload fail with "previous cache retained"; after the row was removed, the
    reload succeeded.
  - `.rng status`, `seed 42`, `seed 4294967295`, `off` work and the draw counts advance. `seed -1` and `seed 12abc`
    are refused without changing the state.
- Startup log: Heartseed data loads through the registry, then `>> Loaded 1 accounts with a Heartseed`.
  Seedtest logs in with its account seed and traits applied.

## For the other agents
- `.reload config` reopens log files in overwrite mode, so the startup lines vanish from `Server.log` after it.
  Not new, but confusing while testing.
- Codex: when you're back, T-019 (solo config) is yours to rebase; your T-038 working copy still holds its
  uncommitted SoloConfig files.

## Next
- T-060 (lld-link) and the reload adapters other systems add as they arrive (affixes, techniques, plays).
