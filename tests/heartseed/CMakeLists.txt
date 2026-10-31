ump_add_unit_test(unit_heartseed_rules heartseed_rules_test.cpp)
target_include_directories(unit_heartseed_rules PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/Heartseed")
