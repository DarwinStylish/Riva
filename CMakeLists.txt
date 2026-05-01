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

  add_executable(riva_tests
    tests/smoke_test.cpp
  )

  target_link_libraries(riva_tests
    PRIVATE
      riva_core
  )

  add_test(NAME riva_smoke_tests COMMAND riva_tests)
endif()
