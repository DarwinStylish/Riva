# Feature Release: Sample Trace Suite & Diagnostic Demo (`feat/samples-demo`)

## Overview

This release expands the Riva sample repository and diagnostic documentation to provide full coverage across all eight builtin Unreal Engine 5 performance stall signatures. With a complete suite of deterministic trace files, engineering teams and CI pipelines can reliably verify end-to-end trace ingestion, baseline hitch detection, and root-cause classification.

## Key Accomplishments

### 1. Complete Sample Trace Repository (`samples/`)
We added seven new JSON trace datasets alongside `spike_shader_compile.json`. Each file simulates a specific Unreal Engine performance bottleneck with stable baseline frames and a targeted hitch frame:
- `spike_pso_miss.json`: Pipeline State Object compilation wait (`STUT_PSO_MISS`).
- `spike_streaming_io.json`: Async loading and Zen IO dispatcher saturation (`STUT_STREAMING_IO`).
- `spike_cpu_game_thread.json`: Game Thread execution bound stall (`STUT_CPU_GT`).
- `spike_cpu_render_thread.json`: Render Thread traversal bottleneck (`STUT_CPU_RT`).
- `spike_rhi_sync.json`: RHI thread presentation synchronization wait (`STUT_RHI_SYNC`).
- `spike_garbage_collection.json`: Synchronous UObject garbage collection pause (`STUT_GC`).
- `spike_gpu_variance_lumen.json`: GPU-bound Lumen and Virtual Shadow Map variance spike (`STUT_GPU_VARIANCE_LUMEN_VSM`).

### 2. Demonstration Guide (`docs/demo.md`)
Created a comprehensive reference guide and walkthrough detailing:
- Step-by-step instructions for running `riva analyze` and `riva validate` from the terminal.
- A complete reference matrix linking each sample trace to its target signature ID, display name, default severity, and primary root cause.
- Detailed case studies showing expected Markdown report extracts and structured JSON output schemas.

### 3. Automated End-to-End Test Verification
Extended the CLI integration test suite (`tests/cli_integration_test.cpp`) to iterate through all eight sample trace files, ensuring that every trace loads cleanly, detects the simulated frame hitch, and accurately matches the expected primary stall classification.

## Verification

The suite has been validated against all project test targets:
```bash
cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure
```
All test suites pass with a 100% success rate.
