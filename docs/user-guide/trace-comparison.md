# Trace Comparison & Regression Testing Guide

The **Trace Compare** API and CLI capability enable game teams to structurally diff two trace analysis results, answering whether a new build introduced regressions, resolved an existing bottleneck, or left the performance baseline unchanged.

---

## How Comparison Works

1. **Primary Finding Isolation**: Comparison strictly operates on `kPrimary` findings identified via signature IDs (`STUT_GC`, `STUT_PSO_MISS`, etc.). This ensures that symptom differences (e.g. cascading secondary stalls) do not trigger false regressions.
2. **Signature ID Matching**: Baseline findings and new trace findings are compared by signature ID.

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
```
