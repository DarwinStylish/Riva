# Verification and Worked Examples

This page is the reproducible, non-video proof path for Riva. Every command
below operates on checked-in source and sample data. Generated reports and logs
are evidence only for the source revision recorded alongside them.

## Current verification boundary

| Capability | Evidence available now | Status |
|---|---|---|
| Standalone C++20 core and CLI | CMake build, CTest, strict warnings, ASan/UBSan CI | Verified by automation |
| JSON trace analysis | Eight deterministic worked samples and report generation | Verified by automation |
| Comparison and budget exit codes | Process-level CLI tests | Verified by automation |
| Unreal plugin source structure | Descriptor and source-contract tests | Verified without Unreal Engine |
| Unreal Engine 5.4 packaging | `RunUAT BuildPlugin` verification script | Requires a real UE 5.4 installation |
| Native `.utrace` frame and timing ingestion | TraceServices implementation | Implemented, pending engine build validation |
| Native named marker extraction | No implementation or proof claimed | Planned work |
| Unreal Insights selection synchronization | No implementation or proof claimed | Planned work |

The distinction between standalone verification and Unreal verification is
intentional. A CMake build cannot establish that an Unreal plugin packages or
loads successfully.

## Reproducible standalone build

Prerequisites are CMake 3.20 or newer and a C++20 compiler.

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

The equivalent explicit commands are:

```bash
cmake -S . -B build/release \
  -DCMAKE_BUILD_TYPE=Release \
  -DRIVA_BUILD_TESTS=ON \
  -DRIVA_BUILD_CLI=ON \
  -DRIVA_WARNINGS_AS_ERRORS=ON
cmake --build build/release --parallel 4
ctest --test-dir build/release --output-on-failure --no-tests=error
```

## Worked example 1: classify a shader-compilation hitch

Input: [`samples/spike_shader_compile.json`](../../samples/spike_shader_compile.json)

```bash
build/release/riva analyze samples/spike_shader_compile.json --format markdown
```

Expected facts:

- Command exit code: `0`.
- Frames analyzed: `3`.
- Hitch count: `1`.
- Primary finding ID: `STUT_SHADER_COMPILE`.
- Supporting observed event: `ShaderCompileWorker blocked frame`.

This sample is synthetic and is labeled as such. It verifies deterministic rule
behavior; it is not presented as a native Unreal capture.

## Worked example 2: enforce a performance budget

Inputs:

- [`samples/sample_budget.json`](../../samples/sample_budget.json)
- [`samples/spike_shader_compile.json`](../../samples/spike_shader_compile.json)

```bash
build/release/riva check-budget \
  --budget samples/sample_budget.json \
  --trace samples/spike_shader_compile.json
```

Expected exit code: `3`, meaning the command completed and the budget gate
detected a breach. Empty budgets and non-positive thresholds are rejected.

## Worked example 3: detect a controlled comparison regression

```bash
build/release/riva compare \
  samples/spike_cpu_game_thread.json \
  samples/spike_shader_compile.json \
  --output controlled-regression.md
```

Expected exit code: `3`. The report records the new primary shader-compilation
classification. Exit code `0` is reserved for a successful comparison with no
detected qualitative or metric regression.

## Sanitizer verification

```bash
cmake --preset sanitizers
cmake --build --preset sanitizers
ctest --preset sanitizers
```

The JSON integer conversion boundary, sparse frame IDs, non-finite telemetry,
empty budgets, and zero-baseline comparisons have dedicated regression tests.

## Collect a reviewer evidence bundle

Run this only from the revision being submitted:

```bash
./scripts/collect-verification-evidence.sh
```

The collector creates an ignored `artifacts/verification/<timestamp>/` folder
containing:

- exact configure, build, and CTest logs;
- an installed standalone CLI, static library, and public-header tree;
- environment and compiler information;
- JSON and Markdown outputs for all eight worked traces;
- a controlled regression report;
- budget and comparison gate logs;
- a status manifest and SHA-256 checksums.

The collector rejects a dirty source tree. `--allow-dirty` exists only for local
development checks. When `UE_ROOT` is not configured, the manifest records the
UE 5.4 package status as `not_run`.

## Unreal Engine 5.4 package verification

On a machine with Unreal Engine 5.4 installed:

```bash
export UE_ROOT=/absolute/path/to/UnrealEngine-5.4
./scripts/verify-ue54-plugin.sh /absolute/path/to/RivaEditor-UE54
```

This invokes Epic's `RunUAT BuildPlugin`, after validating the engine's
`Build.version`. A successful package log and the resulting plugin archive are
the required evidence before Riva is described as UE 5.4 package-verified.
