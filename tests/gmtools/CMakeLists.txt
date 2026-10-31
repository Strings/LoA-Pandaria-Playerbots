ump_add_unit_test(unit_twink_gear twink_gear_test.cpp)
target_sources(unit_twink_gear PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/GMTools/Checkpoints/TwinkGearRules.cpp")
target_include_directories(unit_twink_gear PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/GMTools/Checkpoints")
