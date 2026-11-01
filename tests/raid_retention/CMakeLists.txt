ump_add_unit_test(unit_raid_retention raid_retention_test.cpp)
target_include_directories(unit_raid_retention PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/game/Instances")
