cmake_minimum_required(VERSION 3.20)

project(Riva
  VERSION 0.1.0
  DESCRIPTION "Deterministic Unreal Engine performance diagnostics companion"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RIVA_BUILD_TESTS "Build Riva tests" ON)

add_library(riva_core
  src/analysis_engine.cpp
  src/builtin_signatures.cpp
  src/causal_chain_resolver.cpp
  src/confidence_resolver.cpp
  src/correlation_resolver.cpp
  src/default_analysis.cpp
  src/json_trace_adapter.cpp
  src/json_trace_loader.cpp
  src/normalized_trace.cpp
  src/report_engine.cpp
  src/signature_result.cpp
  src/signature_utils.cpp
  src/spike_detector.cpp
  src/trace_adapter_registry.cpp
  src/version.cpp
)

target_include_directories(riva_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

if(MSVC)
  target_compile_options(riva_core PRIVATE /W4 /permissive-)
else()
  target_compile_options(riva_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(riva
  apps/riva-cli/main.cpp
)

target_link_libraries(riva
  PRIVATE
    riva_core
)

if(RIVA_BUILD_TESTS)
  enable_testing()

  add_executable(riva_smoke_tests
    tests/smoke_test.cpp
  )

  target_link_libraries(riva_smoke_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_smoke_tests COMMAND riva_smoke_tests)

  add_executable(riva_core_engine_tests
    tests/core_engine_test.cpp
  )

  target_link_libraries(riva_core_engine_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_core_engine_tests COMMAND riva_core_engine_tests)

  add_executable(riva_builtin_signatures_tests
    tests/builtin_signatures_test.cpp
  )

  target_link_libraries(riva_builtin_signatures_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_builtin_signatures_tests COMMAND riva_builtin_signatures_tests)

  add_executable(riva_causal_chain_resolver_tests
    tests/causal_chain_resolver_test.cpp
  )

  target_link_libraries(riva_causal_chain_resolver_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_causal_chain_resolver_tests COMMAND riva_causal_chain_resolver_tests)

  add_executable(riva_confidence_resolver_tests
    tests/confidence_resolver_test.cpp
  )

  target_link_libraries(riva_confidence_resolver_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_confidence_resolver_tests COMMAND riva_confidence_resolver_tests)

  add_executable(riva_correlation_resolver_tests
    tests/correlation_resolver_test.cpp
  )

  target_link_libraries(riva_correlation_resolver_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_correlation_resolver_tests COMMAND riva_correlation_resolver_tests)

  add_executable(riva_json_trace_loader_tests
    tests/json_trace_loader_test.cpp
  )

  target_link_libraries(riva_json_trace_loader_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_json_trace_loader_tests COMMAND riva_json_trace_loader_tests)

  add_executable(riva_trace_adapter_tests
    tests/trace_adapter_test.cpp
  )

  target_link_libraries(riva_trace_adapter_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_trace_adapter_tests COMMAND riva_trace_adapter_tests)

  add_executable(riva_report_engine_tests
    tests/report_engine_test.cpp
  )

  target_link_libraries(riva_report_engine_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_report_engine_tests COMMAND riva_report_engine_tests)

  add_executable(riva_cli_integration_tests
    tests/cli_integration_test.cpp
  )

  target_link_libraries(riva_cli_integration_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_cli_integration_tests COMMAND riva_cli_integration_tests)

  add_executable(riva_plugin_structure_tests
    tests/plugin_structure_test.cpp
  )

  target_link_libraries(riva_plugin_structure_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_plugin_structure_tests COMMAND riva_plugin_structure_tests)
endif()
