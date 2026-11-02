ump_add_unit_test(unit_spawn_thinning_rules spawn_thinning_rules_test.cpp)
target_include_directories(unit_spawn_thinning_rules PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/SpawnThinning")
