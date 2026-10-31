# gemini: T-033 (Crash triage)

**Branch:** `gemini/T-033-crash-triage`
**Status:** review

## What changed

- Added `tools/crash-triage/` (`__init__.py`, `triage_core.py`, `crash_triage.py`, `test_crash_triage.py`, `README.md`).
- Separated report logic in `triage_core.py` (parsing `cdb` output, parsing `WheatyExceptionReport` `.txt` crash reports, merging both sources, normalizing worktree/repo paths, classifying threads by subsystem role, diagnosing `E06D7363` mutex self-deadlocks and `C0000005` access violations, and parsing `git log` output) from running `cdb` in `crash_triage.py`.
- Supported both crash dump analysis (`-z <dump>.dmp` with `.symopt+ 0x40` so slightly mismatched/rebuilt `.pdb` symbols next to `worldserver.exe` still resolve) and non-invasive live process attach (`cdb -pv -p <pid>`) for hung servers.
- Made `cdb.exe` location configurable via `--cdb-bin`, `MOP_CDB_BIN` / `CDB_PATH` environment variables, `PATH`, and fallback search across `WindowsApps\Microsoft.WinDbg_*` and `Windows Kits\10\Debuggers\x64`.

## Tested

- Unit tests: `python -m unittest discover -s tools/crash-triage -v` (6 tests covering `parse_cdb_output`, `parse_wheaty_report`, `merge_crash_data`, self-deadlock diagnosis, `C0000005` null-register diagnosis, source path normalization, and `parse_git_log_output`; all pass).
- Real crash dump test: ran `python tools/crash-triage/crash_triage.py "I:\UltimateMoP\.worktrees\claude\build\bin\Crashes\Archive_worldserver.exe_[31-10_15-31-8]"` (read-only). Confirmed the report diagnoses the self-deadlock in `HeartseedMgr::GetAwakenings` (`src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp:984`) <- `HeartseedMgr::EffectiveRank (GetEffectiveRank)` (`HeartseedMgr.cpp:1008`) <- `HeartseedMgr::RefreshTraits` (`HeartseedMgr.cpp:1608`, holding `Local std::lock_guard<std::mutex>`), summarizes the 15 other threads by role, and identifies recent commits touching `HeartseedMgr.cpp` (`0e3ad918 Heartseed: one seed per account (T-049)`).

## SQL applied to `*_gemini`

- None (tooling only).

## For the other agent

- `python tools/crash-triage/crash_triage.py <path-to-Crashes-dir-or-dump>` will automatically pick the newest crash in a `Crashes/` directory or analyze a specific `.dmp` / `.txt` / stem, and `--pid <pid>` attaches non-invasively to a hung `worldserver.exe`.

## Decisions / provisional items

- Combined `cdb` minidump analysis with `WheatyExceptionReport` `.txt` parsing when both exist: if `worldserver.exe` was rebuilt shortly after the crash (as in today's `Archive_worldserver.exe_[31-10_15-31-8]`), `cdb` with `.symopt+ 0x40` still extracts the OS thread IDs and top symbol, while the in-process `WheatyExceptionReport` provides the complete 21-frame unwind and local variable `std::lock_guard<std::mutex>` annotations.
