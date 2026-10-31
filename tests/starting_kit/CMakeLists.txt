ump_add_unit_test(unit_starting_kit starting_kit_test.cpp)
target_include_directories(unit_starting_kit PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/StartingKit")
