# Riva Worked Sample Guide

This is a reproducible CLI walkthrough, not evidence from a live Unreal Engine
session. The eight checked-in JSON traces are small, deterministic fixtures that
exercise known analysis branches. Use captured `.utrace` data and manual Unreal
Insights review to evaluate behavior on a real project.

## Build once

From the repository root:

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

The commands below then use `build/release/riva`. Add `--output <path>` to
preserve a report instead of writing it to standard output.

## Fixture matrix

| Fixture | Expected primary ID | Expected title | Controlled evidence |
|---|---|---|---|
| `spike_shader_compile.json` | `STUT_SHADER_COMPILE` | Shader compilation stall | `ShaderCompileWorker blocked frame` |
| `spike_pso_miss.json` | `STUT_PSO_MISS` | Pipeline state object miss | `PSO compile draw call wait` |
| `spike_streaming_io.json` | `STUT_STREAMING_IO` | Streaming or IO stall | `Asset loading Zen IO dispatcher stall` |
| `spike_cpu_game_thread.json` | `STUT_CPU_GT` | Game thread CPU spike | elevated `game_thread_ms` |
| `spike_cpu_render_thread.json` | `STUT_CPU_RT` | Render thread CPU spike | elevated `render_thread_ms` |
| `spike_rhi_sync.json` | `STUT_RHI_SYNC` | RHI synchronization stall | `Wait for RHI presentation buffer` |
| `spike_garbage_collection.json` | `STUT_GC` | Garbage collection stall | `Collect garbage mark` |
| `spike_gpu_variance_lumen.json` | `STUT_GPU_VARIANCE_LUMEN_VSM` | GPU variance from Lumen or virtual shadow maps | elevated `gpu_ms` |

Each fixture contains three frames with one controlled hitch. The process-level
and integration tests verify loading, frame count, hitch count, primary ID, and
selected evidence for all eight files.

## Example 1: inspect a PSO finding

```bash
build/release/riva analyze samples/spike_pso_miss.json --format markdown
```

The report should contain:

```text
Total Frames Analyzed: 3
Frame Hitch Count: 1
Primary Stall Classification: Pipeline state object miss
ID: STUT_PSO_MISS
event [OBSERVED]: PSO compile draw call wait
```

It also retains the concurrent Render Thread finding as secondary instead of
discarding it. The primary designation is an evidence-based rank; confirmation
steps in the report remain part of the diagnosis.

## Example 2: export machine-readable GPU evidence

```bash
build/release/riva analyze \
  samples/spike_gpu_variance_lumen.json \
  --format json \
  --output gpu-variance-report.json
```

Useful fields for automation include:

```json
{
  "executive_summary": {
    "total_frames_analyzed": 3,
    "hitch_count": 1,
    "primary_stall_classification": "GPU variance from Lumen or virtual shadow maps"
  },
  "findings": [
    {
      "id": "STUT_GPU_VARIANCE_LUMEN_VSM",
      "role": "Primary",
      "affected_thread": "GPU",
      "affected_system": "Rendering"
    }
  ]
}
```

The actual report contains additional evidence, guidance, percentile statistics,
and subsystem score fields. Consumers should parse JSON fields rather than
matching whitespace or key order.

## Example 3: prove the CI gate contract

```bash
build/release/riva check-budget \
  --budget samples/sample_budget.json \
  --trace samples/spike_shader_compile.json
budget_status=$?
test "$budget_status" -eq 3
```

Exit code `3` means analysis completed and the configured gate was breached.
Exit code `1` is an operational or input error, while `0` means the gate passed.
The same distinction applies to `compare`: a detected regression is `3`, not a
generic program failure.

## Generate the complete reviewer packet

```bash
./scripts/collect-verification-evidence.sh
```

From a clean revision this builds with warnings-as-errors, runs the full test
suite, emits Markdown and JSON for every fixture, exercises both gate exit
codes, records the toolchain, and creates SHA-256 checksums. See
[`verification/README.md`](verification/README.md) for the verification boundary
and UE 5.4 packaging command.
