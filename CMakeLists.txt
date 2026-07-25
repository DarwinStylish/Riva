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
  src/confidence_resolver.cpp
  src/normalized_trace.cpp
  src/signature_result.cpp
  src/signature_utils.cpp
  src/spike_detector.cpp
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

if(RIVA_BUILD_TESTS)
  enable_testing()

  add_executable(riva_builtin_signatures_tests
    tests/builtin_signatures_test.cpp
  )
  target_link_libraries(riva_builtin_signatures_tests PRIVATE riva_core)
  add_test(NAME riva_builtin_signatures_tests COMMAND riva_builtin_signatures_tests)

  add_executable(riva_confidence_resolver_tests
    tests/confidence_resolver_test.cpp
  )
  target_link_libraries(riva_confidence_resolver_tests PRIVATE riva_core)
  add_test(NAME riva_confidence_resolver_tests COMMAND riva_confidence_resolver_tests)

  add_executable(riva_core_engine_tests
    tests/core_engine_test.cpp
  )
  target_link_libraries(riva_core_engine_tests PRIVATE riva_core)
  add_test(NAME riva_core_engine_tests COMMAND riva_core_engine_tests)
endif()
