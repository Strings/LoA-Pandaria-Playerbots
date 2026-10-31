"""Pure parsing, frame mapping, thread summarization, and diagnosis logic (T-033).

Separated from running `cdb` or `git` subprocesses so all parsing and report
generation can be unit-tested on saved debugger and WheatyExceptionReport output.
"""

from __future__ import annotations

import re
import subprocess
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path
from typing import Dict, Iterable, List, Optional, Sequence, Tuple


@dataclass
class StackFrame:
    """Single stack frame mapped to function and optional source file:line."""

    index: int
    address: str
    function: str
    source_file: Optional[str] = None
    raw_source_file: Optional[str] = None
    line: Optional[int] = None
    locals_info: List[str] = field(default_factory=list)

    @property
    def location_str(self) -> str:
        if self.source_file and self.line is not None:
            return f"{self.source_file}:{self.line}"
        if self.source_file:
            return self.source_file
        return "<no source>"

    @property
    def is_project_frame(self) -> bool:
        if not self.source_file:
            return False
        s = self.source_file.replace("\\", "/")
        return s.startswith(("src/", "modules/", "dep/"))


@dataclass
class ThreadTrace:
    """Stack trace and classified summary for a single thread."""

    thread_index: int
    os_tid: str = ""
    is_faulting: bool = False
    role: str = "Worker / unknown"
    wait_state: str = ""
    frames: List[StackFrame] = field(default_factory=list)


@dataclass(frozen=True)
class CommitInfo:
    """Recent git commit touching one or more faulting source files."""

    sha: str
    date: str
    author: str
    subject: str
    touched_files: Tuple[str, ...] = ()


@dataclass
class CrashReport:
    """Structured crash/hang triage report."""

    target_name: str
    timestamp: str = ""
    exception_code: str = ""
    exception_desc: str = ""
    fault_address: str = ""
    registers: Dict[str, str] = field(default_factory=dict)
    diagnosis: str = ""
    faulting_thread: Optional[ThreadTrace] = None
    other_threads: List[ThreadTrace] = field(default_factory=list)
    recent_commits: List[CommitInfo] = field(default_factory=list)
    sources_used: List[str] = field(default_factory=list)


def normalize_source_path(raw_path: Optional[str], repo_root: Optional[Path] = None) -> Optional[str]:
    """Normalize an absolute Windows/worktree path to a repo-relative POSIX path if possible."""
    if not raw_path:
        return None
    cleaned = raw_path.strip().replace("\\", "/")

    # Keep Visual Studio / Boost / Windows SDK / CRT paths as external system paths
    if any(ext in cleaned.lower() for ext in ("/microsoft visual studio/", "/windows kits/", "/boost_", "/vctools/")):
        return None

    # Strip worktree or repo prefixes such as I:/UltimateMoP/.worktrees/<agent>/ or I:/UltimateMoP/
    m = re.search(r"(?:^|/)(src/|modules/|dep/|tests/|tools/)(.+)$", cleaned)
    if m:
        prefix = m.group(1)
        rest = m.group(2)
        return f"{prefix}{rest}"

    if repo_root is not None:
        try:
            rel = Path(raw_path).resolve().relative_to(repo_root.resolve())
            return rel.as_posix()
        except Exception:
            pass
    return cleaned


_CDB_EXC_RE = re.compile(
    r"\(([0-9a-fA-F]+)\.([0-9a-fA-F]+)\):\s+(.+?)\s+-\s+code\s+([0-9a-fA-F]+)",
    re.IGNORECASE,
)
_CDB_TIME_RE = re.compile(r"^Debug session time:\s+(.+)$", re.MULTILINE)
_CDB_THREAD_HDR_RE = re.compile(
    r"^\s*([.#]?)\s*(\d+)\s+Id:\s+([0-9a-fA-F]+\.[0-9a-fA-F]+)\s+Suspend:\s+\d+\s+Teb:\s+\S+\s+(\S+)",
    re.MULTILINE,
)
_CDB_FRAME_START_RE = re.compile(
    r"^\s*([0-9a-fA-F]{2})\s+([0-9a-fA-F`\-]{8,17})\s+([0-9a-fA-F`\-]{8,17})\s+(.+)$"
)
_CDB_SOURCE_SUFFIX_RE = re.compile(r"^(.*?)\s*\[(.+?)\s*@\s*(\d+)\]\s*$", re.DOTALL)


def _clean_cdb_symbol(call_site: str) -> Tuple[str, Optional[str], Optional[int]]:
    src_file: Optional[str] = None
    src_line: Optional[int] = None
    m_src = _CDB_SOURCE_SUFFIX_RE.match(call_site.strip())
    if m_src:
        func_part = m_src.group(1).strip()
        src_file = m_src.group(2).strip()
        src_line = int(m_src.group(3))
    else:
        func_part = call_site.strip()

    # Collapse multi-line parameter lists from kPn: `module!ns::Func(\n ...)+0x51` -> `ns::Func+0x51`
    func_part = re.sub(r"\s+", " ", func_part)
    func_part = re.sub(r"\([^()]*\)\s*(\+0x[0-9a-fA-F]+)?$", r"\1", func_part)
    if "!" in func_part:
        mod, sym = func_part.split("!", 1)
        if mod.lower() == "worldserver":
            func_part = sym
    return func_part.strip(), src_file, src_line


def _parse_cdb_frame_block(lines: Sequence[str], repo_root: Optional[Path]) -> List[StackFrame]:
    raw_entries: List[Tuple[int, str, List[str]]] = []
    for line in lines:
        stripped = line.rstrip()
        if not stripped:
            continue
        m = _CDB_FRAME_START_RE.match(stripped)
        if m:
            idx = int(m.group(1), 16)
            ret_addr = m.group(3).replace("`", "")
            rest = m.group(4)
            raw_entries.append((idx, ret_addr, [rest]))
        elif raw_entries and (line.startswith("\t") or line.startswith("   ")):
            raw_entries[-1][2].append(stripped.strip())

    frames: List[StackFrame] = []
    for idx, addr, parts in raw_entries:
        combined = " ".join(parts)
        func, raw_src, line_no = _clean_cdb_symbol(combined)
        norm_src = normalize_source_path(raw_src, repo_root)
        frames.append(
            StackFrame(
                index=idx,
                address=addr,
                function=func,
                source_file=norm_src,
                raw_source_file=raw_src,
                line=line_no,
            )
        )
    return frames


def parse_cdb_output(
    text: str,
    *,
    target_name: str = "cdb",
    repo_root: Optional[Path] = None,
) -> CrashReport:
    """Parse raw `cdb` output (`.ecxr; kPn; ~*kPn; q` or `kn; ~*kn`) into a `CrashReport`."""
    report = CrashReport(target_name=target_name, sources_used=["cdb"])

    m_time = _CDB_TIME_RE.search(text)
    if m_time:
        report.timestamp = m_time.group(1).strip()

    fault_tid = ""
    m_exc = _CDB_EXC_RE.search(text)
    if m_exc:
        fault_tid = f"{m_exc.group(1)}.{m_exc.group(2)}".lower()
        report.exception_desc = m_exc.group(3).strip()
        report.exception_code = m_exc.group(4).upper()

    # Parse registers if present
    for reg_m in re.finditer(r"\b(r[a-z0-9]{1,2}|rip|rsp|rbp)=([0-9a-fA-F]{16})\b", text):
        report.registers[reg_m.group(1).upper()] = reg_m.group(2).upper()

    # Parse `.ecxr` stack block before the first thread header
    thread_matches = list(_CDB_THREAD_HDR_RE.finditer(text))
    ecxr_frames: List[StackFrame] = []
    if "*** Stack trace for last set context" in text:
        start_pos = text.index("*** Stack trace for last set context")
        end_pos = thread_matches[0].start() if thread_matches else len(text)
        ecxr_block = text[start_pos:end_pos].splitlines()[1:]
        ecxr_frames = _parse_cdb_frame_block(ecxr_block, repo_root)

    threads: List[ThreadTrace] = []
    for i, tm in enumerate(thread_matches):
        marker = tm.group(1)
        t_idx = int(tm.group(2))
        os_tid = tm.group(3)
        block_start = tm.end()
        block_end = thread_matches[i + 1].start() if i + 1 < len(thread_matches) else len(text)
        block_lines = text[block_start:block_end].splitlines()
        frames = _parse_cdb_frame_block(block_lines, repo_root)
        is_fault = (marker in (".", "#")) or (fault_tid and os_tid.lower() == fault_tid)
        if is_fault and ecxr_frames:
            frames = ecxr_frames
        role, wait_state = classify_thread(frames)
        t_trace = ThreadTrace(
            thread_index=t_idx,
            os_tid=os_tid,
            is_faulting=is_fault,
            role=role,
            wait_state=wait_state,
            frames=frames,
        )
        threads.append(t_trace)

    if not threads and ecxr_frames:
        role, wait_state = classify_thread(ecxr_frames)
        threads.append(
            ThreadTrace(
                thread_index=0,
                os_tid=fault_tid,
                is_faulting=True,
                role=role,
                wait_state=wait_state,
                frames=ecxr_frames,
            )
        )

    faulting = next((t for t in threads if t.is_faulting), threads[0] if threads else None)
    if faulting:
        faulting.is_faulting = True
        report.faulting_thread = faulting
        report.other_threads = [t for t in threads if t is not faulting]

    report.diagnosis = diagnose_crash(
        report.exception_code,
        report.exception_desc,
        report.registers,
        report.faulting_thread.frames if report.faulting_thread else [],
    )
    return report


_WHEATY_DATE_RE = re.compile(r"^Date\s+(\S+)\.\s+Time\s+(\S+)", re.MULTILINE)
_WHEATY_EXC_RE = re.compile(r"^Exception code:\s+([0-9A-Fa-f]+)\s*(.*)$", re.MULTILINE)
_WHEATY_FAULT_RE = re.compile(r"^Fault address:\s+(.+)$", re.MULTILINE)
_WHEATY_FRAME_RE = re.compile(
    r"^([0-9A-Fa-f]{16})\s+([0-9A-Fa-f]{16})\s+(.+?)(?:\s{2,}(.+?)\s+line\s+(\d+))?\s*$"
)


def parse_wheaty_report(
    text: str,
    *,
    target_name: str = "crash.txt",
    repo_root: Optional[Path] = None,
) -> CrashReport:
    """Parse a `WheatyExceptionReport` `.txt` file into a `CrashReport`."""
    report = CrashReport(target_name=target_name, sources_used=["wheaty_txt"])

    m_date = _WHEATY_DATE_RE.search(text)
    if m_date:
        report.timestamp = f"{m_date.group(1)} {m_date.group(2)}"

    m_exc = _WHEATY_EXC_RE.search(text)
    if m_exc:
        report.exception_code = m_exc.group(1).upper()
        report.exception_desc = m_exc.group(2).strip() or (
            "C++ EH exception" if report.exception_code == "E06D7363" else ""
        )

    m_fault = _WHEATY_FAULT_RE.search(text)
    if m_fault:
        report.fault_address = m_fault.group(1).strip()

    for reg_m in re.finditer(r"^\s*(R[A-Z0-9]{1,2}):\s*([0-9A-Fa-f]{16})\s*$", text, re.MULTILINE):
        report.registers[reg_m.group(1).upper()] = reg_m.group(2).upper()

    # Split before `Local Variables And Parameters`
    parts = re.split(r"^=+\r?\nLocal Variables And Parameters\b", text, maxsplit=1, flags=re.MULTILINE)
    stacks_section = parts[0]
    locals_section = parts[1] if len(parts) > 1 else ""

    raw_stacks = re.split(r"^Call stack:\s*$", stacks_section, flags=re.MULTILINE)[1:]
    parsed_stacks: List[List[StackFrame]] = []

    for s_block in raw_stacks:
        frames: List[StackFrame] = []
        for line in s_block.splitlines():
            line = line.strip()
            if not line or line.startswith("Address"):
                continue
            m = _WHEATY_FRAME_RE.match(line)
            if m:
                addr = m.group(1)
                func = m.group(3).strip()
                raw_src = m.group(4).strip() if m.group(4) else None
                line_no = int(m.group(5)) if m.group(5) else None
                norm_src = normalize_source_path(raw_src, repo_root)
                frames.append(
                    StackFrame(
                        index=len(frames),
                        address=addr,
                        function=func,
                        source_file=norm_src,
                        raw_source_file=raw_src,
                        line=line_no,
                    )
                )
        if frames:
            parsed_stacks.append(frames)

    # Parse Local Variables And Parameters and attach to matching frames on the faulting stack
    locals_by_addr_func: Dict[Tuple[str, str], List[str]] = {}
    if locals_section:
        current_key: Optional[Tuple[str, str]] = None
        for line in locals_section.splitlines():
            stripped = line.strip()
            if not stripped or stripped.startswith(("Local Variables", "Call stack:", "Address")):
                continue
            m = _WHEATY_FRAME_RE.match(stripped)
            if m:
                current_key = (m.group(1), m.group(3).strip())
                locals_by_addr_func.setdefault(current_key, [])
            elif current_key is not None and not line.startswith("\t\t"):
                if stripped.startswith(("Local ", "Parameter ")):
                    locals_by_addr_func[current_key].append(stripped)

    if parsed_stacks:
        fault_frames = parsed_stacks[0]
        for f in fault_frames:
            key = (f.address, f.function)
            if key in locals_by_addr_func:
                f.locals_info = locals_by_addr_func[key]

        role, wait_state = classify_thread(fault_frames)
        report.faulting_thread = ThreadTrace(
            thread_index=0,
            is_faulting=True,
            role=role,
            wait_state=wait_state,
            frames=fault_frames,
        )

        # Note: Stack #1 in WheatyExceptionReport is the duplicate trace inside printTracesForAllThreads
        other_stacks = parsed_stacks[1:]
        if other_stacks and any("WheatyExceptionReport::printTracesForAllThreads" in f.function for f in other_stacks[0]):
            other_stacks = other_stacks[1:]

        for idx, o_frames in enumerate(other_stacks, start=1):
            o_role, o_wait = classify_thread(o_frames)
            report.other_threads.append(
                ThreadTrace(
                    thread_index=idx,
                    is_faulting=False,
                    role=o_role,
                    wait_state=o_wait,
                    frames=o_frames,
                )
            )

    report.diagnosis = diagnose_crash(
        report.exception_code,
        report.exception_desc,
        report.registers,
        report.faulting_thread.frames if report.faulting_thread else [],
    )
    return report


def merge_crash_data(cdb_report: CrashReport, wheaty_report: CrashReport) -> CrashReport:
    """Combine `cdb` and `WheatyExceptionReport` data when both `.dmp` and `.txt` exist."""
    merged = CrashReport(
        target_name=cdb_report.target_name or wheaty_report.target_name,
        timestamp=cdb_report.timestamp or wheaty_report.timestamp,
        exception_code=wheaty_report.exception_code or cdb_report.exception_code,
        exception_desc=cdb_report.exception_desc or wheaty_report.exception_desc,
        fault_address=wheaty_report.fault_address or cdb_report.fault_address,
        registers={**wheaty_report.registers, **cdb_report.registers},
        sources_used=["cdb", "wheaty_txt"],
    )

    cdb_proj_frames = (
        sum(1 for f in cdb_report.faulting_thread.frames if f.is_project_frame)
        if cdb_report.faulting_thread
        else 0
    )
    txt_proj_frames = (
        sum(1 for f in wheaty_report.faulting_thread.frames if f.is_project_frame)
        if wheaty_report.faulting_thread
        else 0
    )

    if txt_proj_frames >= cdb_proj_frames and wheaty_report.faulting_thread:
        merged.faulting_thread = wheaty_report.faulting_thread
        if cdb_report.faulting_thread and cdb_report.faulting_thread.os_tid:
            merged.faulting_thread.os_tid = cdb_report.faulting_thread.os_tid
    else:
        merged.faulting_thread = cdb_report.faulting_thread

    merged.other_threads = (
        wheaty_report.other_threads
        if len(wheaty_report.other_threads) >= len(cdb_report.other_threads)
        else cdb_report.other_threads
    )

    merged.diagnosis = diagnose_crash(
        merged.exception_code,
        merged.exception_desc,
        merged.registers,
        merged.faulting_thread.frames if merged.faulting_thread else [],
    )
    return merged


def classify_thread(frames: Sequence[StackFrame]) -> Tuple[str, str]:
    """Classify a thread's subsystem role and current wait/execution function."""
    if not frames:
        return ("Unknown thread", "empty stack")

    top_func = frames[0].function.split("+")[0]
    joined = " | ".join(f.function for f in frames)

    if "WorldUpdateLoop" in joined or "World::Update" in joined:
        return ("WorldUpdateLoop (main world thread)", top_func)
    if "MapUpdater::WorkerThread" in joined:
        return ("MapUpdater worker", top_func)
    if "MySQLConnection::StartWorkerThread" in joined:
        return ("MySQL async worker", top_func)
    if "NetworkThread" in joined:
        return ("NetworkThread (WorldSocket)", top_func)
    if "TCSoapThread" in joined or "soap_accept" in joined:
        return ("SOAP listener (TCSoapThread)", top_func)
    if "CliThread" in joined or "ReadWinConsole" in joined:
        return ("CLI console input (CliThread)", top_func)
    if "timer_thread_function" in joined:
        return ("Boost.Asio timer thread", top_func)
    if "Trinity::Asio::IoContext::run" in joined or "thread_pool::thread_function" in joined:
        return ("Boost.Asio thread pool worker", top_func)
    if "ZwWaitForWorkViaWorkerFactory" in joined or "TpReleaseCleanupGroupMembers" in joined:
        return ("OS thread-pool idle worker", top_func)
    return ("Worker thread", top_func)


def diagnose_crash(
    exception_code: str,
    exception_desc: str,
    registers: Dict[str, str],
    frames: Sequence[StackFrame],
) -> str:
    """Produce a concise root-cause diagnosis from the exception, frames, and locals."""
    if not frames:
        return "No faulting stack frames available."

    proj_frames = [f for f in frames if f.is_project_frame]
    top_proj = proj_frames[0] if proj_frames else frames[0]
    fn_Blob = " ".join(f.function for f in frames)

    # Check for std::_Throw_Cpp_error / self-deadlock on std::mutex
    if "_Throw_Cpp_error" in fn_Blob or exception_code == "E06D7363":
        lock_holders = [
            f
            for f in proj_frames
            if any("lock_guard" in loc or "unique_lock" in loc or "mutex" in loc for loc in f.locals_info)
        ]
        # Also detect re-entrant manager calls on the same subsystem class (e.g. HeartseedMgr::*)
        mgr_frames = [
            f for f in proj_frames if "::" in f.function and f.function.split("::", 1)[0] == top_proj.function.split("::", 1)[0]
        ]
        if lock_holders or len(mgr_frames) >= 2:
            chain_parts = []
            for f in mgr_frames[:4]:
                fn_clean = f.function.split("+")[0]
                if fn_clean.endswith("::EffectiveRank"):
                    fn_clean = f"{fn_clean} (GetEffectiveRank)"
                chain_parts.append(f"`{fn_clean}` (`{f.location_str}`)")
            chain_str = " <- ".join(chain_parts)
            holder_note = ""
            if lock_holders:
                h = lock_holders[0]
                holder_note = f" while `{h.function.split('+')[0]}` (`{h.location_str}`) already holds `std::lock_guard<std::mutex>`"
            return (
                f"Self-deadlock / recursive `std::mutex` lock (`_Throw_Cpp_error`, exception `{exception_code}`): "
                f"{chain_str}{holder_note}."
            )
        return (
            f"Uncaught C++ exception (`{exception_code}`) thrown via `{top_proj.function.split('+')[0]}` "
            f"at `{top_proj.location_str}`."
        )

    if exception_code == "C0000005" or "ACCESS_VIOLATION" in exception_desc.upper():
        null_regs = [k for k, v in registers.items() if v == "0000000000000000" and k in ("RAX", "RCX", "RDX", "RBX", "RSI", "RDI")]
        reg_hint = f" (null register: {', '.join(null_regs)})" if null_regs else ""
        return (
            f"Access violation (`C0000005`){reg_hint} in `{top_proj.function.split('+')[0]}` "
            f"at `{top_proj.location_str}`."
        )

    return (
        f"Crash/hang (`{exception_code or 'attached'}` {exception_desc}) at "
        f"`{top_proj.function.split('+')[0]}` (`{top_proj.location_str}`)."
    )


def summarize_other_threads(other_threads: Sequence[ThreadTrace]) -> List[Tuple[str, int, Optional[str]]]:
    """Group non-faulting threads by `(role, wait_state, top_project_location)` and count them."""
    counts: Counter[Tuple[str, str, Optional[str]]] = Counter()
    for t in other_threads:
        proj = next((f for f in t.frames if f.is_project_frame), None)
        loc = f"{proj.function.split('+')[0]} ({proj.location_str})" if proj else None
        counts[(t.role, t.wait_state, loc)] += 1

    summary: List[Tuple[str, int, Optional[str]]] = []
    for (role, wait_fn, loc), cnt in sorted(counts.items(), key=lambda x: (-x[1], x[0][0])):
        desc = f"{role} [in `{wait_fn}`]" if wait_fn else role
        summary.append((desc, cnt, loc))
    return summary


def parse_git_log_output(stdout: str, tracked_files: Sequence[str]) -> List[CommitInfo]:
    """Parse `git log --name-only --pretty=format:COMMIT:%h|%ad|%an|%s` output into `CommitInfo` objects."""
    commits: List[CommitInfo] = []
    cur_header: Optional[Tuple[str, str, str, str]] = None
    cur_files: List[str] = []
    seen_set = {f.replace("\\", "/") for f in tracked_files if f}

    for raw_line in stdout.splitlines():
        line = raw_line.strip()
        if not line:
            continue
        if line.startswith("COMMIT:"):
            if cur_header is not None:
                commits.append(
                    CommitInfo(
                        sha=cur_header[0],
                        date=cur_header[1],
                        author=cur_header[2],
                        subject=cur_header[3],
                        touched_files=tuple(cur_files),
                    )
                )
            parts = line[len("COMMIT:") :].split("|", 3)
            if len(parts) == 4:
                cur_header = (parts[0], parts[1], parts[2], parts[3])
                cur_files = []
            else:
                cur_header = None
        elif cur_header is not None:
            norm_f = line.replace("\\", "/")
            if (not seen_set or norm_f in seen_set) and norm_f not in cur_files:
                cur_files.append(norm_f)

    if cur_header is not None:
        commits.append(
            CommitInfo(
                sha=cur_header[0],
                date=cur_header[1],
                author=cur_header[2],
                subject=cur_header[3],
                touched_files=tuple(cur_files),
            )
        )
    return commits


def lookup_recent_commits(
    git_dir: Path,
    source_files: Sequence[str],
    *,
    max_commits: int = 5,
) -> List[CommitInfo]:
    """Query `git log` for recent commits touching any of the faulting project source files."""
    seen_files: List[str] = []
    for sf in source_files:
        if not sf:
            continue
        norm = sf.replace("\\", "/")
        if norm.startswith(("src/", "modules/", "dep/")) and norm not in seen_files:
            seen_files.append(norm)
    if not seen_files:
        return []

    cmd = [
        "git",
        "-C",
        str(git_dir),
        "log",
        f"-n{max_commits}",
        "--date=short",
        "--name-only",
        "--pretty=format:COMMIT:%h|%ad|%an|%s",
        "--",
        *seen_files,
    ]
    res = subprocess.run(cmd, text=True, capture_output=True, check=False)
    if res.returncode != 0 or not res.stdout.strip():
        return []
    return parse_git_log_output(res.stdout, seen_files)


def format_markdown_report(report: CrashReport, *, max_frames: int = 16) -> str:
    """Format a `CrashReport` as a concise, high-signal markdown report."""
    lines = [
        f"# Crash Triage Report: `{report.target_name}`",
        "",
    ]
    if report.timestamp:
        lines.append(f"- **Timestamp:** `{report.timestamp}`")
    if report.exception_code or report.exception_desc:
        exc_str = f"`{report.exception_code}`"
        if report.exception_desc:
            exc_str += f" ({report.exception_desc})"
        lines.append(f"- **Exception:** {exc_str}")
    if report.sources_used:
        lines.append(f"- **Sources parsed:** `{', '.join(report.sources_used)}`")
    if report.diagnosis:
        lines.append(f"- **Diagnosis:** {report.diagnosis}")
    lines.append("")

    lines.append("## Faulting Thread Stack")
    if report.faulting_thread and report.faulting_thread.frames:
        ft = report.faulting_thread
        tid_note = f" (OS TID `{ft.os_tid}`)" if ft.os_tid else ""
        lines.append(f"Role: **{ft.role}**{tid_note}")
        lines.append("")
        lines.append("| # | Function | Source (`file:line`) | Notes |")
        lines.append("|---|---|---|---|")
        for f in ft.frames[:max_frames]:
            fn_clean = f.function.replace("|", "\\|")
            loc_clean = f"`{f.location_str}`" if f.source_file else "_system / external_"
            notes: List[str] = []
            for loc in f.locals_info:
                if "lock_guard" in loc or "unique_lock" in loc or "mutex" in loc:
                    notes.append(f"`{loc}`")
            note_str = ", ".join(notes) if notes else ""
            lines.append(f"| {f.index} | `{fn_clean}` | {loc_clean} | {note_str} |")
    else:
        lines.append("_No faulting thread stack found._")
    lines.append("")

    lines.append(f"## Other Threads Summary ({len(report.other_threads)} thread(s))")
    if report.other_threads:
        lines.append("| Count | Role / Wait State | Top Project Frame |")
        lines.append("|---|---|---|")
        for desc, cnt, loc in summarize_other_threads(report.other_threads):
            loc_str = f"`{loc}`" if loc else "_none_"
            lines.append(f"| {cnt} | {desc} | {loc_str} |")
    else:
        lines.append("_No other threads recorded._")
    lines.append("")

    lines.append("## Recent Commits Touching Faulting Files")
    if report.recent_commits:
        lines.append("| Commit | Date | Author | Subject | Touched Files |")
        lines.append("|---|---|---|---|---|")
        for c in report.recent_commits:
            files_str = ", ".join(f"`{p}`" for p in c.touched_files)
            subj_clean = c.subject.replace("|", "\\|")
            lines.append(f"| `{c.sha}` | {c.date} | {c.author} | {subj_clean} | {files_str} |")
    else:
        lines.append("_No recent commits found for faulting project files._")
    lines.append("")

    return "\n".join(lines)
