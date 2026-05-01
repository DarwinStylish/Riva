# Riva

Riva is a deterministic performance diagnostics companion for Unreal Insights.

## Rules

- Pure C++20 core
- No Unreal headers in include/riva
- No manual .utrace parsing
- Deterministic analysis only

## Build

cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
