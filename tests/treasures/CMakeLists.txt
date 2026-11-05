ump_add_unit_test(unit_treasure_rules treasure_rules_test.cpp)
target_include_directories(unit_treasure_rules PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/Treasures")
