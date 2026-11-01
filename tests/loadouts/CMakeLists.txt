ump_add_unit_test(unit_loadout_rules loadout_rules_test.cpp)
target_include_directories(unit_loadout_rules PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/Loadouts")
