# Riva Performance Diagnostics Demo & Reference Guide

Riva is a deterministic performance intelligence layer designed for Unreal Engine 5. It ingests trace data, detects frame hitches against rolling median baselines, and classifies root causes into actionable engineering signatures with evidence and Unreal-native confirmation guidance.

This guide demonstrates how to use the command-line tool (`riva`) to analyze the builtin sample trace repository located in `samples/`.

---

## 1. Quick Start

To build and run the Riva command-line diagnostic tool from the root workspace directory:

```bash
# Configure and build the project
cmake -S . -B build && cmake --build build -j

# Run analysis against a sample trace in human-readable Markdown format
./build/riva analyze samples/spike_shader_compile.json --format markdown

# Run analysis and output machine-readable JSON to a file
./build/riva analyze samples/spike_pso_miss.json --format json --output report.json
```

---

## 2. Sample Trace Reference Suite

The repository includes eight deterministic sample trace datasets in `samples/`, covering the full spectrum of Unreal Engine 5 performance bottlenecks:

| Sample Trace File | Target Signature ID | Display Classification | Default Severity | Primary Root Cause Simulated |
| :--- | :--- | :--- | :--- | :--- |
| `spike_shader_compile.json` | `STUT_SHADER_COMPILE` | Shader compilation stall | Critical | Runtime shader compilation blocking gameplay or editor interaction. |
| `spike_pso_miss.json` | `STUT_PSO_MISS` | Pipeline state object cache miss | Critical | Draw-time hitch caused by missing Pipeline State Object precaching. |
| `spike_streaming_io.json` | `STUT_STREAMING_IO` | Asset streaming / I/O saturation | Warning | Async loading and Zen IO dispatcher saturation during package streaming. |
| `spike_cpu_game_thread.json` | `STUT_CPU_GT` | Game Thread execution bound stall | Warning | Game Thread execution bound tick (e.g. heavy Blueprint or animation logic). |
| `spike_cpu_render_thread.json` | `STUT_CPU_RT` | Render Thread execution bound stall | Warning | Render Thread traversal and draw call preparation bottleneck. |
| `spike_rhi_sync.json` | `STUT_RHI_SYNC` | RHI thread / GPU synchronization stall | Warning | CPU thread waiting on GPU presentation buffer or fence synchronization. |
| `spike_garbage_collection.json` | `STUT_GC` | Garbage collection stall | Critical | Synchronous UObject mark and sweep garbage collection pause. |
| `spike_gpu_variance_lumen.json` | `STUT_GPU_VARIANCE_LUMEN_VSM` | Virtual Shadow Maps / Lumen lighting GPU variance spike | Warning | GPU-bound lighting and virtual shadow map pass expansion. |

---

## 3. Demonstration Walkthroughs

### Case 1: Pipeline State Object Cache Miss (`STUT_PSO_MISS`)
When a new material permutation or mesh is rendered without precached PSOs, the RHI thread stalls while compiling pipeline states.

**Command:**
```bash
./build/riva analyze samples/spike_pso_miss.json --format markdown
```

**Expected Report Extract:**
```markdown
# Riva Performance Diagnostic Report

## Executive Summary
- **Source**: spike_pso_miss_sample
- **Total Frames Analyzed**: 3
- **Frame Hitch Count**: 1
- **Primary Stall Classification**: Pipeline state object cache miss

## Detailed Findings

### [Critical] Pipeline state object cache miss
- **Confidence**: 79.0%
- **Frame Index**: 2

#### Supporting Evidence
- **event**: PSO compile draw call wait
- **frame_ms**: 52.00 ms
- **baseline_ms**: 16.00 ms
- **delta_ms**: 36.00 ms

#### Recommended Next Steps
- Review PSO precaching coverage for the affected content.
- Check whether a new material, mesh, or render state appeared in the spike window.
- Validate PSO cache generation for the target platform.

#### How to Confirm in Unreal Insights
- Open the RHI and rendering tracks in Unreal Insights.
- Look for PSO creation or pipeline state compilation overlapping the hitch.
```

---

### Case 2: Synchronous Garbage Collection (`STUT_GC`)
Unreal Engine periodic garbage collection can trigger multi-millisecond synchronous pauses if object allocation churn is high.

**Command:**
```bash
./build/riva analyze samples/spike_garbage_collection.json --format markdown
```

**Expected Report Extract:**
```markdown
### [Critical] Garbage collection stall
- **Confidence**: 83.0%
- **Frame Index**: 2

#### Supporting Evidence
- **event**: Collect Garbage Mark and Sweep
- **frame_ms**: 65.00 ms
- **baseline_ms**: 16.00 ms
- **delta_ms**: 49.00 ms

#### Recommended Next Steps
- Inspect UObject allocation and GC cadence.
- Avoid triggering expensive GC during active gameplay.
- Review object lifetime churn near the spike.

#### How to Confirm in Unreal Insights
- Open GC markers in Unreal Insights.
- Confirm mark, sweep, or purge work overlaps the spike window.
```

---

### Case 3: GPU Variance & Lumen / VSM Spike (`STUT_GPU_VARIANCE_LUMEN_VSM`)
Heavy scene complexity or lighting changes can cause GPU execution time to exceed the target frame budget.

**Command:**
```bash
./build/riva analyze samples/spike_gpu_variance_lumen.json --format json
```

**Expected JSON Output Structure:**
```json
{
  "executive_summary": {
    "source_name": "spike_gpu_variance_lumen_sample",
    "total_frames_analyzed": 3,
    "hitch_count": 1,
    "primary_stall_classification": "Virtual Shadow Maps / Lumen lighting GPU variance spike"
  },
  "findings": [
    {
      "id": "STUT_GPU_VARIANCE_LUMEN_VSM",
      "name": "Virtual Shadow Maps / Lumen lighting GPU variance spike",
      "severity": "Warning",
      "confidence": 0.74,
      "evidence": [
        { "key": "frame_ms", "value": "55.00 ms" },
        { "key": "gpu_ms", "value": "52.00 ms" },
        { "key": "baseline_ms", "value": "16.00 ms" }
      ]
    }
  ]
}
```

---

## 4. Verification and Validation Commands

To validate the integrity of a trace file without generating a full diagnostic report, use the `validate` subcommand:

```bash
./build/riva validate samples/spike_rhi_sync.json
```
If valid, the CLI returns exit code `0` and confirms schema compliance. If invalid or malformed, it returns a non-zero exit code and diagnostic error message.
