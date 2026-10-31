"""Unit tests for tools/crash-triage parsing, frame mapping, thread classification, and git log parsing (T-033)."""

from __future__ import annotations

import unittest

try:
    from .triage_core import (
        CommitInfo,
        StackFrame,
        classify_thread,
        diagnose_crash,
        format_markdown_report,
        merge_crash_data,
        normalize_source_path,
        parse_cdb_output,
        parse_git_log_output,
        parse_wheaty_report,
        summarize_other_threads,
    )
except ImportError:
    from triage_core import (  # type: ignore[no-redef]
        CommitInfo,
        StackFrame,
        classify_thread,
        diagnose_crash,
        format_markdown_report,
        merge_crash_data,
        normalize_source_path,
        parse_cdb_output,
        parse_git_log_output,
        parse_wheaty_report,
        summarize_other_threads,
    )


SAVED_CDB_DUMP_OUTPUT = r"""
Microsoft (R) Windows Debugger Version 10.0.27920.1001 AMD64
Loading Dump File [I:\UltimateMoP\.worktrees\claude\build\bin\Crashes\Archive_worldserver.exe_[31-10_15-31-8].dmp]
User Mini Dump File: Only registers, stack and portions of memory are available

Debug session time: Sat Oct 31 15:31:08.000 2026 (UTC - 4:00)
System Uptime: not available
Process Uptime: 0 days 0:00:35.000
(48d4.c304): C++ EH exception - code e06d7363 (first/second chance not available)
rax=00007ff86ba0b54f rbx=00007ff9056c2950 rcx=0000017b86d62470
rdx=0000017b86da7ae0 rsi=00000032544fe0f0 rdi=0000000019930520
rip=00007ff932065369 rsp=00000032544fdf90 rbp=0000000000000005
  *** Stack trace for last set context - .thread/.cxr resets it
 # Child-SP          RetAddr               Call Site
00 00000032`544fdf90 00007ff9`055e55a9     KERNELBASE!RaiseException+0x69
01 00000032`544fe070 00007ff9`05668906     VCRUNTIME140!_CxxThrowException+0x99
02 00000032`544fe0d0 00007ff7`8c7744b1     MSVCP140!std::_Throw_Cpp_error+0x46
03 00000032`544fe130 00000000`00000000     worldserver!HeartseedMgr::GetAwakenings(
			unsigned int accountId = <Value unavailable error>)+0xe1 [I:\UltimateMoP\.worktrees\claude\src\server\scripts\Custom\Heartseed\HeartseedMgr.cpp @ 986]

#  0  Id: 48d4.c304 Suspend: 0 Teb: 00000032`543c5000 Unfrozen
 # Child-SP          RetAddr               Call Site
00 00000032`544fbee8 00000032`00000000     ntdll!NtGetContextThread+0x14
01 00000032`544fbef0 00000002`544f0100     0x00000032`00000000

   1  Id: 48d4.5a04 Suspend: 0 Teb: 00000032`543c7000 Unfrozen
 # Child-SP          RetAddr               Call Site
00 00000032`545ff468 00007ff9`348fd407     ntdll!ZwWaitForWorkViaWorkerFactory+0x14
01 00000032`545ff470 00007ff9`344e7374     ntdll!TpReleaseCleanupGroupMembers+0x747
02 00000032`545ff778 00007ff9`348fcc91     KERNEL32!BaseThreadInitThunk+0x14
03 00000032`545ff7a8 00000000`00000000     ntdll!RtlUserThreadStart+0x21

   9  Id: 48d4.cb58 Suspend: 0 Teb: 00000032`543d7000 Unfrozen
 # Child-SP          RetAddr               Call Site
00 00000032`54eff7a8 00007ff9`349155f1     ntdll!NtWaitForAlertByThreadId+0x14
01 00000032`54eff7b0 00007ff9`320aee79     ntdll!RtlSleepConditionVariableSRW+0x131
02 00000032`54eff838 00007ff9`056676ea     KERNELBASE!SleepConditionVariableSRW+0x29
03 00000032`54eff878 00007ff7`8d2bef97     MSVCP140!_Cnd_wait+0x2a
04 00000032`54eff8a8 00007ff7`8d2be84f     worldserver!MapUpdater::WorkerThread(void)+0x57 [I:\UltimateMoP\.worktrees\claude\src\server\game\Maps\MapUpdater.cpp @ 111]
quit:
"""


SAVED_WHEATY_REPORT = r"""Revision: Pandaria 5.4.8 Rev: 0 RelWithDebInfo Hash: Archive (Win64, little-endian)
Date 31:10:2026. Time 15:31 
//=====================================================
*** Hardware ***
Processor: Intel(R) Core(TM) i5-9600K CPU @ 3.70GHz

//=====================================================
Exception code: E06D7363 
Fault address:  00007FF932065369 01:0000000000024369 C:\WINDOWS\System32\KERNELBASE.dll

Registers:
RAX:00007FF86BA0B54F
RBX:00007FF9056C2950
RCX:0000017B86D62470
RDX:0000017B86DA7AE0

Call stack:
Address   Frame     Function      SourceFile
00007FF932065369  00000032544FE060  RaiseException+69
00007FF9055E55A9  00000032544FE0C0  _CxxThrowException+99
00007FF905668906  00000032544FE120  ?_Throw_Cpp_error@std@@YAXH@Z+46
00007FF78C7744B1  00000032544FE150  HeartseedMgr::GetAwakenings+E1  I:\UltimateMoP\.worktrees\claude\src\server\scripts\Custom\Heartseed\HeartseedMgr.cpp line 984
00007FF78C7736B3  00000032544FE190  HeartseedMgr::EffectiveRank+33  I:\UltimateMoP\.worktrees\claude\src\server\scripts\Custom\Heartseed\HeartseedMgr.cpp line 1008
00007FF78C77DC8F  00000032544FE2D0  HeartseedMgr::RefreshTraits+30F  I:\UltimateMoP\.worktrees\claude\src\server\scripts\Custom\Heartseed\HeartseedMgr.cpp line 1608
00007FF78C77D935  00000032544FE300  HeartseedMgr::Refresh+15  I:\UltimateMoP\.worktrees\claude\src\server\scripts\Custom\Heartseed\HeartseedMgr.cpp line 1561
00007FF78C74A49D  00000032544FE330  heartseed_playerscript::OnLogin+3D  I:\UltimateMoP\.worktrees\claude\src\server\scripts\Custom\Heartseed\heartseed_scripts.cpp line 69
00007FF78B6C22E3  00000032544FF030  WorldUpdateLoop+5E3  I:\UltimateMoP\.worktrees\claude\src\server\worldserver\Main.cpp line 497

Call stack:
Address   Frame     Function      SourceFile
00007FF93494F3F4  00000032544FBF50  NtGetContextThread+14
00007FF78B6DBD17  00000032544FC4B0  WheatyExceptionReport::printTracesForAllThreads+B7  I:\UltimateMoP\.worktrees\claude\src\common\Debugging\WheatyExceptionReport.cpp line 464

Call stack:
Address   Frame     Function      SourceFile
00007FF934950FD4  0000003254EFF7E0  NtWaitForAlertByThreadId+14
00007FF78D2BEF97  0000003254EFF900  MapUpdater::WorkerThread+57  I:\UltimateMoP\.worktrees\claude\src\server\game\Maps\MapUpdater.cpp line 111

Call stack:
Address   Frame     Function      SourceFile
00007FF93494D6C4  0000003254AFFBD0  NtRemoveIoCompletion+14
00007FF78B734A9E  0000003254AFFE80  std::thread::_Invoke<std::tuple<`MySQLConnection::StartWorkerThread'::`2'::<lambda_1> >,0>+3E  C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.39.33519\include\thread line 60
========================
Local Variables And Parameters

Call stack:
Address   Frame     Function      SourceFile
00007FF78C7744B1  00000032544FE150  HeartseedMgr::GetAwakenings+E1  I:\UltimateMoP\.worktrees\claude\src\server\scripts\Custom\Heartseed\HeartseedMgr.cpp line 984

00007FF78C7736B3  00000032544FE190  HeartseedMgr::EffectiveRank+33  I:\UltimateMoP\.worktrees\claude\src\server\scripts\Custom\Heartseed\HeartseedMgr.cpp line 1008
	Parameter  Player* player = 0x17c9a73f020

00007FF78C77DC8F  00000032544FE2D0  HeartseedMgr::RefreshTraits+30F  I:\UltimateMoP\.worktrees\claude\src\server\scripts\Custom\Heartseed\HeartseedMgr.cpp line 1608
	Local  std::lock_guard<std::mutex>
		<user defined>  _MyMutex = 0x7ff78f583540
"""


class TestCrashTriageCore(unittest.TestCase):
    def test_normalize_source_path(self) -> None:
        self.assertEqual(
            normalize_source_path(
                r"I:\UltimateMoP\.worktrees\claude\src\server\scripts\Custom\Heartseed\HeartseedMgr.cpp"
            ),
            "src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp",
        )
        self.assertIsNone(
            normalize_source_path(
                r"C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.39.33519\include\xmemory"
            )
        )
        self.assertIsNone(normalize_source_path(r"C:\local\boost_1_85_0\boost\asio\impl\thread_pool.ipp"))

    def test_parse_cdb_output(self) -> None:
        report = parse_cdb_output(SAVED_CDB_DUMP_OUTPUT, target_name="test_dump")
        self.assertEqual(report.exception_code, "E06D7363")
        self.assertIn("C++ EH exception", report.exception_desc)
        self.assertIsNotNone(report.faulting_thread)
        assert report.faulting_thread is not None
        self.assertEqual(report.faulting_thread.os_tid, "48d4.c304")
        self.assertEqual(len(report.faulting_thread.frames), 4)
        top_proj = report.faulting_thread.frames[3]
        self.assertEqual(top_proj.function, "HeartseedMgr::GetAwakenings+0xe1")
        self.assertEqual(
            top_proj.source_file,
            "src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp",
        )
        self.assertEqual(top_proj.line, 986)
        self.assertEqual(len(report.other_threads), 2)

    def test_parse_wheaty_report_and_self_deadlock_diagnosis(self) -> None:
        report = parse_wheaty_report(SAVED_WHEATY_REPORT, target_name="Archive_test")
        self.assertEqual(report.exception_code, "E06D7363")
        self.assertIsNotNone(report.faulting_thread)
        assert report.faulting_thread is not None
        self.assertEqual(report.faulting_thread.role, "WorldUpdateLoop (main world thread)")

        funcs = [f.function for f in report.faulting_thread.frames]
        self.assertIn("HeartseedMgr::GetAwakenings+E1", funcs)
        self.assertIn("HeartseedMgr::EffectiveRank+33", funcs)
        self.assertIn("HeartseedMgr::RefreshTraits+30F", funcs)

        # Verify local mutex lock_guard was attached to RefreshTraits
        refresh_frame = next(
            f for f in report.faulting_thread.frames if "RefreshTraits" in f.function
        )
        self.assertTrue(any("lock_guard<std::mutex>" in loc for loc in refresh_frame.locals_info))

        # Verify self-deadlock diagnosis mentions GetEffectiveRank / EffectiveRank and GetAwakenings
        self.assertIn("Self-deadlock", report.diagnosis)
        self.assertIn("HeartseedMgr::EffectiveRank (GetEffectiveRank)", report.diagnosis)
        self.assertIn("HeartseedMgr::GetAwakenings", report.diagnosis)
        self.assertIn("std::lock_guard<std::mutex>", report.diagnosis)

        # Verify duplicate printTracesForAllThreads stack was skipped
        self.assertEqual(len(report.other_threads), 2)
        roles = {t.role for t in report.other_threads}
        self.assertEqual(roles, {"MapUpdater worker", "MySQL async worker"})

    def test_merge_crash_data_and_markdown_report(self) -> None:
        cdb_rep = parse_cdb_output(SAVED_CDB_DUMP_OUTPUT, target_name="Archive_test")
        txt_rep = parse_wheaty_report(SAVED_WHEATY_REPORT, target_name="Archive_test")
        merged = merge_crash_data(cdb_rep, txt_rep)
        merged.recent_commits = [
            CommitInfo(
                sha="0e3ad918",
                date="2026-10-31",
                author="TChavez",
                subject="Heartseed: one seed per account (T-049)",
                touched_files=("src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp",),
            )
        ]

        self.assertEqual(merged.faulting_thread.os_tid, "48d4.c304")
        md = format_markdown_report(merged)
        self.assertIn("HeartseedMgr::EffectiveRank (GetEffectiveRank)", md)
        self.assertIn("src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp:1008", md)
        self.assertIn("0e3ad918", md)
        self.assertIn("MapUpdater worker", md)

    def test_access_violation_diagnosis(self) -> None:
        frames = [
            StackFrame(
                index=0,
                address="00007FF700001000",
                function="Unit::HasAura+0x24",
                source_file="src/server/game/Entities/Unit/Unit.cpp",
                line=5120,
            )
        ]
        diag = diagnose_crash(
            "C0000005",
            "Access violation - code c0000005",
            {"RCX": "0000000000000000", "RAX": "0000017B12345678"},
            frames,
        )
        self.assertIn("Access violation (`C0000005`)", diag)
        self.assertIn("null register: RCX", diag)
        self.assertIn("Unit::HasAura", diag)
        self.assertIn("src/server/game/Entities/Unit/Unit.cpp:5120", diag)

    def test_parse_git_log_output(self) -> None:
        sample_log = (
            "COMMIT:0e3ad918|2026-10-31|TChavez|Heartseed: one seed per account (T-049)\n"
            "\n"
            "src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp\n"
            "src/server/scripts/Custom/Heartseed/heartseed_scripts.cpp\n"
            "COMMIT:74940f10|2026-10-31|TChavez|Heartseed: add the path-choice window (T-023)\n"
            "\n"
            "src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp\n"
        )
        commits = parse_git_log_output(
            sample_log,
            ["src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp"],
        )
        self.assertEqual(len(commits), 2)
        self.assertEqual(commits[0].sha, "0e3ad918")
        self.assertEqual(
            commits[0].touched_files,
            ("src/server/scripts/Custom/Heartseed/HeartseedMgr.cpp",),
        )


if __name__ == "__main__":
    unittest.main()
