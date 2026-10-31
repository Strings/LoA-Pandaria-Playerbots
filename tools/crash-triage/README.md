# Crash Triage (`tools/crash-triage/`)

Produces a concise crash or hang triage report for `worldserver`:

1. **Root-cause diagnosis:** exception code, null-register hint (for `C0000005`), or re-entrant `std::mutex` self-deadlock chain (for `E06D7363` / `_Throw_Cpp_error`).
2. **Faulting thread stack:** mapped to repo-relative `source_file:line`, with mutex lock holders highlighted from local frame variables.
3. **Other threads summary:** all non-faulting threads classified by subsystem role (`WorldUpdateLoop`, `MapUpdater worker`, `MySQL async worker`, `NetworkThread`, `TCSoapThread`, `CliThread`, `Boost.Asio`, OS thread pool) and grouped by wait state.
4. **Recent commits:** `git log` commits that most recently touched the faulting project source files.

## Architecture

- [`triage_core.py`](triage_core.py): Pure, subprocess-free parsing, frame normalization, thread classification, crash diagnosis, git log output parsing, and Markdown formatting.
- [`crash_triage.py`](crash_triage.py): CLI entry point that locates `cdb.exe` and `.pdb` symbols, runs `cdb` on `.dmp` files or attaches non-invasively (`cdb -pv -p <pid>`) to a live hung process, reads companion `WheatyExceptionReport` `.txt` files, queries `git log`, and outputs Markdown or JSON.
- [`test_crash_triage.py`](test_crash_triage.py): Unit tests for `triage_core.py` on saved `cdb` and `WheatyExceptionReport` outputs.

## Configuration & Arguments

`cdb.exe` is never hardcoded. It is resolved in order:
1. `--cdb-bin <path>`
2. `MOP_CDB_BIN` or `CDB_PATH` environment variable
3. `PATH` (`shutil.which("cdb")`)
4. Auto-search in `C:\Program Files\WindowsApps\Microsoft.WinDbg_*\amd64\cdb.exe` and `Windows Kits\10\Debuggers\x64\cdb.exe`

Symbols (`.pdb`) are automatically located next to `worldserver.exe` (one directory up from `build/bin/Crashes/`) or can be overridden via `--symbol-dir`.

| Flag | Default | Description |
|---|---|---|
| `target` | _(positional)_ | Path to a `.dmp` file, `.txt` crash report, crash stem, or `build/bin/Crashes/` directory (picks newest crash) |
| `--pid` | `None` | Attach non-invasively (`cdb -pv -p <pid>`) to a live hung `worldserver` process |
| `--cdb-bin` | `$MOP_CDB_BIN` / search | Explicit path to `cdb.exe` |
| `--symbol-dir` | parent of `Crashes/` | Directory containing `worldserver.pdb` and `worldserver.exe` |
| `--git-dir` | worktree root | Repository root used for `git log` commit lookup |
| `--max-commits` | `5` | Maximum number of recent commits to list for faulting files |
| `--json` | `False` | Emit JSON instead of Markdown |
| `-o`, `--output` | `-` (stdout) | Output file path, or `-` for stdout |

## Example Runs

Triage a specific crash archive (or pass the `build/bin/Crashes/` folder to triage the latest crash):

```powershell
python tools/crash-triage/crash_triage.py "I:\UltimateMoP\.worktrees\claude\build\bin\Crashes\Archive_worldserver.exe_[31-10_15-31-8]"
```

Attach non-invasively to a hung `worldserver.exe` process by PID:

```powershell
python tools/crash-triage/crash_triage.py --pid 18644 --symbol-dir build/bin
```

Run unit tests:

```powershell
python -m unittest discover -s tools/crash-triage -v
```
