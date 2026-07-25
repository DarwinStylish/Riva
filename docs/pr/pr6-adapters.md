# PR6 - Trace Adapters

## Summary

Adds the first trace adapter layer for loading traces through a stable adapter interface.

## Included

- ITraceAdapter interface
- TraceAdapterResult
- JsonTraceAdapter
- TraceAdapterRegistry
- CreateDefaultTraceAdapterRegistry helper
- CreateDefaultAnalysisEngine helper
- Adapter unit tests

## Architecture Discipline

- Pure C++20
- No Unreal headers
- JSON loading stays behind an adapter
- Unsupported formats fail cleanly
- .utrace is intentionally not claimed yet
- Default analysis uses registered builtin signatures

## Verification

cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/riva version
./build/riva --help
