cmake_minimum_required(VERSION 3.20)

project(Riva
  VERSION 0.2.0
  DESCRIPTION "Deterministic Unreal Engine performance diagnostics companion"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(RIVA_BUILD_TESTS "Build Riva tests" ON)

add_library(riva_core
  ue/Plugins/RivaEditor/Source/RivaCore/Private/analysis_engine.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/budget.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/builtin_signatures.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/causal_chain_resolver.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/confidence_resolver.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/correlation_resolver.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/default_analysis.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/json_trace_adapter.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/json_trace_loader.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/json_utils.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/normalized_trace.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/performance_score.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/report_engine.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/signature_result.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/signature_utils.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/spike_detector.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/trace_adapter_registry.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/trace_comparator.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/trace_statistics.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/trace_synthesizer.cpp
  ue/Plugins/RivaEditor/Source/RivaCore/Private/version.cpp
)

target_include_directories(riva_core
  PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ue/Plugins/RivaEditor/Source/RivaCore/Public
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

  add_executable(riva_budget_tests tests/budget_test.cpp)
  target_link_libraries(riva_budget_tests PRIVATE riva_core)
  add_test(NAME riva_budget_tests COMMAND riva_budget_tests)

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

  add_executable(riva_trace_comparator_tests
    tests/trace_comparator_test.cpp
  )

  target_link_libraries(riva_trace_comparator_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_trace_comparator_tests COMMAND riva_trace_comparator_tests)

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

  add_executable(riva_trace_statistics_tests
    tests/trace_statistics_test.cpp
  )

  target_link_libraries(riva_trace_statistics_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_trace_statistics_tests COMMAND riva_trace_statistics_tests)

  add_executable(riva_trace_synthesizer_tests
    tests/trace_synthesizer_test.cpp
  )

  target_link_libraries(riva_trace_synthesizer_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_trace_synthesizer_tests COMMAND riva_trace_synthesizer_tests)

  add_executable(riva_performance_score_tests
    tests/performance_score_test.cpp
  )

  target_link_libraries(riva_performance_score_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_performance_score_tests COMMAND riva_performance_score_tests)

  add_executable(riva_pathology_validation_tests
    tests/pathology_validation_test.cpp
  )

  target_link_libraries(riva_pathology_validation_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_pathology_validation_tests COMMAND riva_pathology_validation_tests)
endif()
