# Feature Release: TraceServices Frame Loader and `.utrace` Ingestion (`feat/traceservices-loader`)

## Overview

This release expands `FRivaTraceService` to ingest native Unreal Insights trace recordings (`.utrace` files). By isolating Unreal Engine's `TraceServices` module behind our architectural service boundary, the plugin extracts frame timings and markers from native recordings, degrades gracefully when marker providers are unavailable, and converts extracted sessions into standard C++ `riva::NormalizedTrace` objects for deterministic diagnostic analysis.

## Key Accomplishments

### 1. TraceServices Abstraction and Graceful Degradation
- Updated `RivaEditor.Build.cs` to declare dependency modules `"TraceServices"`, `"TraceLog"`, and `"TraceAnalysis"`.
- Expanded `FRivaTraceService` with `LoadAndAnalyzeUTrace()` and `ExtractNormalizedTraceFromUTrace()` to handle native Unreal Insights session ingestion.
- Implemented marker provider availability checks (`bMarkerProviderAvailable` in `FRivaNormalizedTraceSummary`): if bookmark or marker providers are missing from a session, the loader logs a warning and degrades gracefully to frame timing extraction without failing ingestion.

### 2. Unified Ingestion and Sample Integration
- Created a unified routing entry point `FRivaTraceService::LoadAndAnalyzeTrace()`, automatically directing `.utrace` files to `LoadAndAnalyzeUTrace()` and `.json` files to `LoadAndAnalyzeJsonTrace()`.
- Added a native sample recording dataset `samples/sample_session.utrace`.
- Updated `SRivaPanel` toolbar actions (`Open Trace...` and `Analyze`) to reference the exact sample filenames present in the repository (`spike_shader_compile.json`, `spike_pso_miss.json`, `spike_streaming_io.json`, and `sample_session.utrace`).

### 3. Automated Structure and Loader Verification
- Expanded `tests/plugin_structure_test.cpp` with `TestTraceServicesLoader()`, validating TraceServices module dependencies, summary structure declarations, graceful marker degradation handling, and sample file rotation.

## Verification

The complete build and test suite has been executed across all target suites:
```bash
cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure
```
All test suites pass with a 100% success rate across 9 target test suites.
