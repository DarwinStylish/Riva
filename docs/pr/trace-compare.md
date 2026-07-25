# Release: Trace Compare

## Overview
This PR concludes the **Studio Diagnostics** phase by introducing the **Trace Compare** API. This capability enables Riva to structurally diff two `AnalysisResult` objects, answering whether a new build introduced regressions, validated an improvement, or left the performance landscape unchanged relative to a baseline trace.

## Core Changes
- **`ITraceComparator`**: Added a new core API that generates a `ComparisonResult` containing three vectors: `regressions`, `improvements`, and `unchanged`.
- **Primary Finding Isolation**: The comparison strictly operates on `kPrimary` findings identified via signature IDs, ensuring that symptom differences (e.g. cascading secondary stalls) do not trigger false regressions.

## Verification
- Wrote `riva_trace_comparator_tests` to simulate baseline vs. new trace permutations.
- Validated correct detection of new issues (`STUT_SHADER_COMPILE` regression), resolved issues (`STUT_STREAMING_IO` improvement), and persistent issues (`STUT_GC` unchanged).
- CTest suite successfully executes all tests.
