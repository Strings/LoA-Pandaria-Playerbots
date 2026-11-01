ump_add_unit_test(unit_capture_rules
  capture_rules_test.cpp
  "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/Capture/CaptureRules.cpp"
)
target_include_directories(unit_capture_rules PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts"
  "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/Capture"
)
