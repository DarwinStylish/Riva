# PR2 - Core Engine

## Summary

Adds the first deterministic RivaCore analysis model.

## Included

- NormalizedTrace model
- Frame model
- TraceEvent model
- Severity, Evidence, and Finding model
- Rolling median baseline helper
- Spike detection engine
- ISignature interface
- Core unit tests

## Architecture Discipline

- Pure C++20
- No Unreal headers
- STL-only public core types
- Deterministic ordering
- No randomness
- No wall-clock behavior

## Verification

cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/riva version
./build/riva --help
