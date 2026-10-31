# C++ unit tests

CTest plus a small exception-based assertion helper, with no added third-party
library. Tests use production pure logic directly, without worldserver, databases,
game data or bot population. UMP_CHECK stays active in Release (unlike assert).

Fast standalone build (only unit test executables):

```
cmake -S tests -B build/unit-tests -DBUILD_TESTING=ON
cmake --build build/unit-tests --config Release
ctest --test-dir build/unit-tests -C Release -L unit --output-on-failure
```

On Windows choose the existing Visual Studio generator if your shell does not
already have a compiler configured. These commands do not build servers or need
Boost/OpenSSL/MySQL. BUILD_TESTING is ON for this standalone tests project.

The main project exposes BUILD_TESTING, OFF by default. Enable it during normal
configuration, then build a named unit target (e.g. unit_time_value) and run CTest
from that build root. Main-project configuration still needs the normal toolchain
dependencies; the standalone route avoids those for pure logic work.

## Add a suite

Create tests/<module>/CMakeLists.txt:

```
ump_add_unit_test(unit_my_rules my_rules_test.cpp)
target_sources(unit_my_rules PRIVATE ../../path/to/pure_rules.cpp)
target_include_directories(unit_my_rules PRIVATE ../../path/to)
```

Include TestMain.h in exactly one translation unit and implement void RunTests().
Call UMP_CHECK(expression) for independently specified input/output contracts.
Split unrelated rule sets into separate executables so CTest reports failures
individually. Every suite's CMakeLists is discovered automatically; no root build
edit is needed. Tests must not require running servers or mutate saves. Additional
target libraries can be linked explicitly when a suite needs existing dependencies.

common/time_value_test.cpp is the example: it exercises the real TimeValue's
precision/truncation, elapsed-time arithmetic, comparisons and zero boundary.
Test fixtures use duration values, not game timers or real sleeps.

The separate tests/regression suite (T-035) covers real headless server behavior;
it is not run implicitly by CTest unit targets.
