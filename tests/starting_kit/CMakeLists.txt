ump_add_unit_test(unit_starting_kit starting_kit_test.cpp)
target_include_directories(unit_starting_kit PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/StartingKit")
target_sources(unit_starting_kit PRIVATE ../../src/server/game/Scripting/FirstLoginHooks.cpp)
target_include_directories(unit_starting_kit PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/game/Scripting")
