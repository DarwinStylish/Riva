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
option(RIVA_BUILD_CLI "Build the Riva command-line tool" ON)
option(RIVA_BUILD_FUZZERS "Build Riva fuzzing targets" OFF)
option(RIVA_ENABLE_SANITIZERS "Enable AddressSanitizer and UndefinedBehaviorSanitizer" OFF)
option(RIVA_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)

if(MSVC)
  add_compile_options(/W4 /permissive-)
  if(RIVA_WARNINGS_AS_ERRORS)
    add_compile_options(/WX)
  endif()
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
  if(RIVA_WARNINGS_AS_ERRORS)
    add_compile_options(-Werror)
  endif()
endif()

if(RIVA_ENABLE_SANITIZERS)
  if(MSVC)
    message(FATAL_ERROR "RIVA_ENABLE_SANITIZERS currently requires GCC or Clang")
  endif()
  add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
endif()

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

if(RIVA_BUILD_CLI)
  add_executable(riva
    apps/riva-cli/main.cpp
  )

  target_link_libraries(riva
    PRIVATE
      riva_core
  )

endif()

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
  set_tests_properties(riva_cli_integration_tests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  )

  if(RIVA_BUILD_CLI)
    add_test(
      NAME riva_cli_process_tests
      COMMAND "${CMAKE_COMMAND}"
        "-DRIVA_EXECUTABLE=$<TARGET_FILE:riva>"
        "-DRIVA_SOURCE_DIR=${CMAKE_CURRENT_SOURCE_DIR}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/tests/cli_process_test.cmake"
    )
  endif()

  add_executable(riva_plugin_structure_tests
    tests/plugin_structure_test.cpp
  )

  target_link_libraries(riva_plugin_structure_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_plugin_structure_tests COMMAND riva_plugin_structure_tests)
  set_tests_properties(riva_plugin_structure_tests PROPERTIES
    WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
  )

  if(UNIX)
    add_test(
      NAME riva_ue54_verify_requires_engine
      COMMAND bash -c "unset UE_ROOT; '${CMAKE_CURRENT_SOURCE_DIR}/scripts/verify-ue54-plugin.sh'"
    )
    set_tests_properties(riva_ue54_verify_requires_engine PROPERTIES WILL_FAIL TRUE)

    add_test(
      NAME riva_fuzz_demo_integrity
      COMMAND bash "${CMAKE_CURRENT_SOURCE_DIR}/tests/fuzz_demo_test.sh"
    )
    set_tests_properties(riva_fuzz_demo_integrity PROPERTIES
      WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
    )
  endif()

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

if(RIVA_BUILD_FUZZERS)
  if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "RIVA_BUILD_FUZZERS requires a Clang compiler with libFuzzer")
  endif()

  add_executable(riva_fuzz_json_trace_loader
    fuzz/targets/fuzz_json_trace_loader.cpp
  )

  target_link_libraries(riva_fuzz_json_trace_loader
    PRIVATE
      riva_core
  )

  target_compile_options(riva_fuzz_json_trace_loader
    PRIVATE
      -fsanitize=fuzzer,address,undefined
      -fno-omit-frame-pointer
      -g
  )

  target_link_options(riva_fuzz_json_trace_loader
    PRIVATE
      -fsanitize=fuzzer,address,undefined
  )
endif()

include(GNUInstallDirs)
install(
  TARGETS riva_core
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
)
if(RIVA_BUILD_CLI)
  install(TARGETS riva RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}")
endif()
install(
  DIRECTORY ue/Plugins/RivaEditor/Source/RivaCore/Public/riva
  DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
  FILES_MATCHING PATTERN "*.hpp"
)
