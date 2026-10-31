ump_add_unit_test(unit_mounted_speed mounted_speed_test.cpp)
target_include_directories(unit_mounted_speed PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/game/Entities/Unit")
