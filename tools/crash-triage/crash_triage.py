#!/usr/bin/env python3
"""Worldserver crash and hang triage CLI (T-033).

Reads `.dmp` and `.txt` crash reports from `build/bin/Crashes/` (or attaches
non-invasively to a live hung process via `cdb -pv -p <pid>`), maps the
faulting thread's stack to `source:line`, summarizes the other threads, and
lists recent git commits that touched the faulting source files.
"""

from __future__ import annotations

import argparse
import glob
import json
import os
import shutil
import subprocess
import sys
from dataclasses import asdict
from pathlib import Path
from typing import List, Optional, Sequence, Tuple

try:
    from .triage_core import (
        CommitInfo,
        CrashReport,
        format_markdown_report,
        lookup_recent_commits,
        merge_crash_data,
        parse_cdb_output,
        parse_wheaty_report,
    )
except ImportError:
    from triage_core import (  # type: ignore[no-redef]
        CommitInfo,
        CrashReport,
        format_markdown_report,
        lookup_recent_commits,
        merge_crash_data,
        parse_cdb_output,
        parse_wheaty_report,
    )


def repo_root() -> Path:
    return Path(__file__).resolve().parents[2]


def find_cdb_binary(explicit: Optional[str] = None) -> Optional[str]:
    """Find `cdb.exe` via `--cdb-bin`, env vars (`MOP_CDB_BIN`, `CDB_PATH`), PATH, or WindowsApps/SDK search."""
    direct_candidates = [
        explicit,
        os.environ.get("MOP_CDB_BIN"),
        os.environ.get("CDB_PATH"),
        shutil.which("cdb"),
        shutil.which("cdb.exe"),
    ]
    for c in direct_candidates:
        if c and Path(c).is_file():
            return str(Path(c))

    # Search WindowsApps WinDbg packages and Windows Kits Debuggers
    prog_files = os.environ.get("ProgramFiles", r"C:\Program Files")
    prog_files_x86 = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    search_patterns = [
        os.path.join(prog_files, "WindowsApps", "Microsoft.WinDbg_*_x64__8wekyb3d8bbwe", "amd64", "cdb.exe"),
        os.path.join(prog_files, "WindowsApps", "Microsoft.WinDbg_*", "amd64", "cdb.exe"),
        os.path.join(prog_files_x86, "Windows Kits", "10", "Debuggers", "x64", "cdb.exe"),
        os.path.join(prog_files, "Windows Kits", "10", "Debuggers", "x64", "cdb.exe"),
    ]
    for pattern in search_patterns:
        matches = sorted(glob.glob(pattern), reverse=True)
        for m in matches:
            if Path(m).is_file():
                return m
    return None


def resolve_crash_files(target: Path) -> Tuple[str, Optional[Path], Optional[Path]]:
    """Resolve a `.dmp`, `.txt`, crash stem, or `Crashes/` directory into `(name, dmp_path, txt_path)`."""
    if target.is_dir():
        candidates = sorted(
            list(target.glob("*.dmp")) + list(target.glob("*.txt")),
            key=lambda p: p.stat().st_mtime,
            reverse=True,
        )
        if not candidates:
            raise FileNotFoundError(f"No .dmp or .txt crash files found in directory: {target}")
        stem_path = candidates[0].with_suffix("")
    elif target.suffix.lower() in (".dmp", ".txt"):
        stem_path = target.with_suffix("")
    else:
        stem_path = target

    dmp_path = Path(f"{stem_path}.dmp")
    txt_path = Path(f"{stem_path}.txt")
    has_dmp = dmp_path.is_file()
    has_txt = txt_path.is_file()
    if not has_dmp and not has_txt:
        raise FileNotFoundError(
            f"Neither {dmp_path} nor {txt_path} exists for target '{target}'"
        )
    return stem_path.name, (dmp_path if has_dmp else None), (txt_path if has_txt else None)


def find_symbol_dir(crash_file: Optional[Path], explicit: Optional[Path] = None) -> Optional[Path]:
    """Locate the directory containing `worldserver.pdb` / `worldserver.exe`."""
    if explicit and explicit.is_dir():
        return explicit
    env_sym = os.environ.get("MOP_SYMBOL_DIR")
    if env_sym and Path(env_sym).is_dir():
        return Path(env_sym)
    if crash_file is not None:
        parent = crash_file.resolve().parent
        # Typical layout: build/bin/Crashes/<dump>.dmp -> symbols in build/bin/ or build/bin/RelWithDebInfo/
        for candidate in (
            parent,
            parent.parent,
            parent.parent / "RelWithDebInfo",
            parent.parent / "Debug",
            parent.parent / "Release",
        ):
            if candidate.is_dir() and any(candidate.glob("*.pdb")):
                return candidate
        if parent.parent.is_dir():
            return parent.parent
    return None


def run_cdb_dump(cdb_bin: str, dmp_path: Path, symbol_dir: Optional[Path], timeout_sec: int = 30) -> str:
    """Run `cdb.exe` on a `.dmp` file and return stdout."""
    cmd = [cdb_bin, "-z", str(dmp_path)]
    if symbol_dir is not None:
        cmd.extend(["-y", str(symbol_dir), "-i", str(symbol_dir)])
    cmd.extend([
        "-lines",
        "-c",
        ".symopt+ 0x40; .reload /f worldserver.exe; .ecxr; kPn; ~*kPn; q",
    ])
    res = subprocess.run(cmd, text=True, capture_output=True, timeout=timeout_sec, check=False)
    if not res.stdout and res.returncode != 0:
        raise RuntimeError(res.stderr.strip() or f"cdb exited with {res.returncode}")
    return res.stdout


def run_cdb_attach(cdb_bin: str, pid: int, symbol_dir: Optional[Path], timeout_sec: int = 30) -> str:
    """Attach non-invasively (`-pv`) to a live hung process with `cdb.exe` and dump all thread stacks."""
    cmd = [cdb_bin, "-pv", "-p", str(pid)]
    if symbol_dir is not None:
        cmd.extend(["-y", str(symbol_dir), "-i", str(symbol_dir)])
    cmd.extend([
        "-lines",
        "-c",
        ".symopt+ 0x40; .reload; ~*kPn; q",
    ])
    res = subprocess.run(cmd, text=True, capture_output=True, timeout=timeout_sec, check=False)
    if not res.stdout and res.returncode != 0:
        raise RuntimeError(res.stderr.strip() or f"cdb exited with {res.returncode}")
    return res.stdout


def triage_target(
    *,
    target: Optional[Path] = None,
    pid: Optional[int] = None,
    cdb_bin: Optional[str] = None,
    symbol_dir: Optional[Path] = None,
    git_dir: Optional[Path] = None,
    max_commits: int = 5,
) -> CrashReport:
    """Run crash triage on a crash target (`.dmp`/`.txt`/stem/folder) or live `--pid`."""
    root = git_dir or repo_root()
    resolved_cdb = find_cdb_binary(cdb_bin)

    if pid is not None:
        if not resolved_cdb:
            raise RuntimeError("cdb.exe not found; set MOP_CDB_BIN or pass --cdb-bin to attach to --pid")
        sym_dir = find_symbol_dir(None, symbol_dir)
        cdb_out = run_cdb_attach(resolved_cdb, pid, sym_dir)
        report = parse_cdb_output(cdb_out, target_name=f"pid:{pid}", repo_root=root)
    else:
        if target is None:
            raise ValueError("Either a crash target path or --pid is required")
        name, dmp_path, txt_path = resolve_crash_files(target)
        sym_dir = find_symbol_dir(dmp_path or txt_path, symbol_dir)

        cdb_report: Optional[CrashReport] = None
        txt_report: Optional[CrashReport] = None

        if dmp_path is not None and resolved_cdb is not None:
            try:
                cdb_out = run_cdb_dump(resolved_cdb, dmp_path, sym_dir)
                cdb_report = parse_cdb_output(cdb_out, target_name=name, repo_root=root)
            except Exception:
                if txt_path is None:
                    raise

        if txt_path is not None:
            txt_content = txt_path.read_text(encoding="utf-8", errors="replace")
            txt_report = parse_wheaty_report(txt_content, target_name=name, repo_root=root)

        if cdb_report and txt_report:
            report = merge_crash_data(cdb_report, txt_report)
        elif txt_report:
            report = txt_report
        elif cdb_report:
            report = cdb_report
        else:
            raise RuntimeError(
                f"Could not analyze {target} (cdb.exe not found and no .txt report present)"
            )

    if report.faulting_thread:
        fault_files = [
            f.source_file
            for f in report.faulting_thread.frames
            if f.source_file and f.is_project_frame
        ]
        report.recent_commits = lookup_recent_commits(root, fault_files, max_commits=max_commits)

    return report


def parse_args(argv: Optional[Sequence[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "target",
        nargs="?",
        help="Path to a .dmp file, .txt crash report, crash stem, or Crashes/ directory",
    )
    parser.add_argument(
        "--pid",
        type=int,
        default=None,
        help="Attach non-invasively (cdb -pv) to a live hung worldserver PID",
    )
    parser.add_argument(
        "--cdb-bin",
        default=None,
        help="Path to cdb.exe (default: MOP_CDB_BIN / CDB_PATH env var or auto-detected WinDbg/SDK)",
    )
    parser.add_argument(
        "--symbol-dir",
        type=Path,
        default=None,
        help="Directory containing worldserver.pdb and worldserver.exe (default: auto-detected next to binary)",
    )
    parser.add_argument(
        "--git-dir",
        type=Path,
        default=repo_root(),
        help="Git repository directory for commit lookup (default: current worktree root)",
    )
    parser.add_argument(
        "--max-commits",
        type=int,
        default=5,
        help="Maximum recent commits to show for faulting files (default: 5)",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Emit JSON report instead of Markdown",
    )
    parser.add_argument(
        "-o",
        "--output",
        default="-",
        help="Output file path, or '-' for stdout (default: '-')",
    )
    return parser.parse_args(argv)


def main(argv: Optional[Sequence[str]] = None) -> int:
    args = parse_args(argv)
    if not args.target and args.pid is None:
        print("error: specify a crash file/stem/directory or --pid <pid>", file=sys.stderr)
        return 2

    try:
        report = triage_target(
            target=Path(args.target) if args.target else None,
            pid=args.pid,
            cdb_bin=args.cdb_bin,
            symbol_dir=args.symbol_dir,
            git_dir=args.git_dir,
            max_commits=args.max_commits,
        )
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    if args.json:
        output_text = json.dumps(asdict(report), indent=2) + "\n"
    else:
        output_text = format_markdown_report(report)

    if args.output == "-":
        sys.stdout.write(output_text)
    else:
        out_path = Path(args.output)
        out_path.parent.mkdir(parents=True, exist_ok=True)
        out_path.write_text(output_text, encoding="utf-8")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
