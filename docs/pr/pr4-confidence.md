# PR4 - Confidence Resolution

## Summary

Adds deterministic confidence calibration and primary/secondary finding resolution.

## Included

- AnalysisResult model
- ResolvedFinding model
- FindingRole primary/secondary classification
- ConfidenceResolver
- AnalysisEngine orchestration layer
- Tests for sorting, filtering, conflict resolution, and full analysis execution

## Architecture Discipline

- Pure C++20
- No Unreal headers
- Deterministic sorting
- Conservative confidence cap
- Weak unsupported findings are dropped
- Conflicting same-frame findings are retained as secondary findings

## Verification

cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/riva version
./build/riva --help
