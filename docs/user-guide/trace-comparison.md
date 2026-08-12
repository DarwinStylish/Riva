# Trace Comparison & Regression Testing Guide

The **Trace Compare** API and CLI capability enable game teams to structurally diff two trace analysis results, answering whether a new build introduced regressions, resolved an existing bottleneck, or left the performance baseline unchanged.

---

## How Comparison Works

1. **Quantitative Metric Diffs**: Computes `FTraceStatistics` (P50, P90, P95, P99, Hitch %, per-subsystem P95) for both baseline and new traces. Generates `FMetricDelta` entries with configurable regression thresholds.
2. **Primary Finding Isolation**: Comparison strictly operates on `kPrimary` findings identified via signature IDs (`STUT_GC`, `STUT_PSO_MISS`, etc.). This ensures that symptom differences (e.g. cascading secondary stalls) do not trigger false regressions.
3. **Signature ID Matching**: Baseline findings and new trace findings are compared by signature ID to track resolved, regressed, or persistent issues.

---

## Finding Classifications

| Category | Definition | Meaning |
|---|---|---|
| **Regressions** | Present in `new_trace`, NOT in `baseline` | **New performance issue introduced.** Requires investigation before merging code/content. |
| **Improvements** | Present in `baseline`, NOT in `new_trace` | **Performance bottleneck resolved.** Validates optimization work. |
| **Unchanged** | Present in BOTH `baseline` and `new_trace` | **Persistent issue.** Bottleneck remains unchanged across runs. |

---

## Running Comparison

### CLI Usage

```bash
./build/riva compare <baseline_trace> <new_trace> [options]
```

#### Example Output Report

```markdown
# Riva Performance Diagnostic Report - Trace Comparison

## Executive Summary
- **Regressions**: 1
- **Improvements**: 1
- **Unchanged**: 0

## Regressions

### 1. [CRITICAL] Pipeline state object miss (ID: STUT_PSO_MISS)
- **Role**: Primary
- **Confidence**: 88.0%
- **Frame Index**: 2
- **Time Window**: 30000 us - 86000 us

#### Evidence Breakdown
- `event`: PSO compile draw call wait
- `frame_ms`: 52.00 ms

## Improvements

### 1. [CRITICAL] Shader compilation stall (ID: STUT_SHADER_COMPILE)
- **Role**: Primary
- **Confidence**: 90.0%
- **Frame Index**: 2
- **Time Window**: 30000 us - 82000 us

## Unchanged Findings

No unchanged findings.

## Metric Summary

| Metric | Baseline | New | Delta | Change |
|---|---|---|---|---|
| P50 Frame Time | 16.20 ms | 16.20 ms | +0.00 ms | +0.0% |
| P90 Frame Time | 41.64 ms | 39.24 ms | -2.40 ms | -5.8% |
| P95 Frame Time | 44.82 ms | 42.12 ms | -2.70 ms | -6.0% |
| P99 Frame Time | 47.36 ms | 44.42 ms | -2.94 ms | -6.2% |
| Hitch % | 33.33% | 33.33% | +0.00% | +0.0% |
| Game Thread P95 | 38.72 ms | 11.72 ms | -27.00 ms | -69.7% |
| Render Thread P95 | 11.51 ms | 7.91 ms | -3.60 ms | -31.3% |
| GPU P95 | 12.73 ms | 10.09 ms | -2.64 ms | -20.7% |
```
