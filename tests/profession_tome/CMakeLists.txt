ump_add_unit_test(unit_profession_tome profession_tome_rules_test.cpp)
target_include_directories(unit_profession_tome PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/ProfessionTome"
    "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/Professions")
