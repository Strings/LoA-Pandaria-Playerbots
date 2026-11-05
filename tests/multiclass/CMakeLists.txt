ump_add_unit_test(unit_multiclass_rules multiclass_rules_test.cpp)
target_include_directories(unit_multiclass_rules PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/Multiclass")
