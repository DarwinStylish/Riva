# Release: TraceServices Marker Event Extraction

## Overview
This release enhances the Unreal Engine plugin service boundary (`FRivaTraceService`) to extract verified marker events directly from TraceServices, empowering the diagnostic engine to analyze garbage collection, asset loading, file I/O, shader compilation, and RHI wait intervals.

## Core Changes
- **Graceful Marker Degradation:** The ingestion pipeline now checks for the active presence of TraceServices bookmark and marker providers. If providers are unavailable, the plugin falls back cleanly to pure frame timing analysis without synthesizing artificial marker data.
- **Reliable Event Extraction:** When active, the trace ingestion layer extracts specific, reliable event categories (`GC`, `AsyncLoading`, `IO`, `ShaderCompile`, `RHIWait`) as `riva::TraceEvent` structures associated with each frame.
- **C++20 Header Alignment:** Internal header inclusions across the Unreal Engine plugin boundary have been aligned strictly to `.hpp` extensions to match the core C++20 STL library (`json_trace_loader.hpp`, `analysis_engine.hpp`, `report_engine.hpp`).

## Verification
- Added `TestMarkerEvents()` to `plugin_structure_test.cpp` to validate TraceServices marker logic and graceful degradation.
- Validated execution of all unit test suites in strict CMake environments to guarantee deterministic fallback behavior when TraceServices libraries are unlinked.
