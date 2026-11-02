ump_add_unit_test(unit_telemetry_core
  telemetry_core_test.cpp
  "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/Telemetry/TelemetryCore.cpp"
)
target_include_directories(unit_telemetry_core PRIVATE
  "${CMAKE_CURRENT_LIST_DIR}/../../src/server/scripts/Custom/Telemetry"
)
