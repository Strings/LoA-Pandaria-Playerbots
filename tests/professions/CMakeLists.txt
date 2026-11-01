ump_add_unit_test(unit_professions profession_rules_test.cpp)
target_include_directories(unit_professions PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/Professions")
