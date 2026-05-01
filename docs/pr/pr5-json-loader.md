# PR5 - JSON Loader

## Summary

Adds strict JSON ingestion for NormalizedTrace test traces.

## Included

- JsonTraceLoader public API
- Strict JSON parser for Riva trace schema
- Schema validation
- Unknown-field rejection by default
- Optional unknown-field allowance for compatibility
- Event and metadata loading
- Sample JSON trace
- Loader unit tests

## Architecture Discipline

- Pure C++20
- No Unreal headers
- No unsafe parsing shortcuts
- Deterministic schema validation
- Robust Status-based error reporting

## Verification

cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/riva version
./build/riva --help
