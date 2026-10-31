ump_add_unit_test(unit_currency_limits currency_limits_test.cpp)
target_include_directories(unit_currency_limits PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/game/World")
