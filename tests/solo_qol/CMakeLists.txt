ump_add_unit_test(unit_solo_qol qol_policy_test.cpp)
target_include_directories(unit_solo_qol PRIVATE "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/SoloQoL")
