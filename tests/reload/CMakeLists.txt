ump_add_unit_test(unit_reload_registry reload_registry_test.cpp)
ump_add_unit_test(unit_seeded_random seeded_random_test.cpp)
target_include_directories(unit_reload_registry PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/Reload")
target_include_directories(unit_seeded_random PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/../../src/common/Utilities"
  "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/Reload")
find_package(Threads REQUIRED)
target_link_libraries(unit_reload_registry PRIVATE Threads::Threads)
target_link_libraries(unit_seeded_random PRIVATE Threads::Threads)
set(UMP_RELOAD_TEST_TIMEOUT 30 CACHE STRING "Timeout for reload/RNG concurrency unit tests in seconds")
set_tests_properties(unit_reload_registry unit_seeded_random PROPERTIES TIMEOUT "${UMP_RELOAD_TEST_TIMEOUT}")
