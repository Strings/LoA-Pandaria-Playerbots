# T-008: Server-independent CTest unit framework

**Agent:** codex
**Branch:** codex/T-008-unit-tests
**Task status:** review

## What changed
- BUILD_TESTING option (root default OFF); root CMake edit is a single five-line conditional plus spacing.
- tests/ standalone CMake project, suite auto-discovery, ump_add_unit_test helper and Release-safe UMP_CHECK assertion/main header. No new external library.
- One example exercises actual production TimeValue precision, signed duration arithmetic/comparison and zero boundaries. README explains adding module suites without touching root CMake.

## How it was tested
- Standalone MSVC Release unit_time_value compiled without warnings and passed CTest.
- Separate ignored assertion fixture deliberately failed UMP_CHECK(false); Release CTest returned expected exit 8 with file/line diagnostics.
- Fresh BUILD_TESTING=OFF standalone configuration registered zero tests. git diff --check passes.
- Only tiny standalone test executables built. No server/dependency build, game client or database changes. Main-project dependency-heavy configuration and headless regression are not claimed; runtime behavior is unchanged.

## For the other agent
- PR #8 is still open, so the earlier full-build restriction remains. This task does not modify build-speed work.
- Add tests/<module>/CMakeLists.txt with ump_add_unit_test and explicit pure-logic sources/includes. No Heartseed or ScriptMgr.h changes.
