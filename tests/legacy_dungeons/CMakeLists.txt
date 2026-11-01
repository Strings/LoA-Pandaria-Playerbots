ump_add_unit_test(unit_legacy_dungeons policy_test.cpp)
target_include_directories(unit_legacy_dungeons PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/LegacyDungeons")
