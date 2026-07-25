# Release: Multi-Spike Correlation

## Overview
This PR initiates the **Studio Diagnostics** phase by introducing advanced heuristics that detect systemic patterns across an entire trace instead of just evaluating isolated incidents. It resolves the problem where frequently occurring hitches (e.g., periodic GC sweeps or clustering IO hits) would flood the UI with repetitive identical findings.

## Core Changes
- **`ICorrelationResolver` Architecture**: Added the new `CorrelationResolver` stage to the core analysis engine pipeline, specifically designed to process findings *after* initial confidence resolution.
- **Clustering Logic**: `DefaultCorrelationResolver` groups findings of the same signature and role if they occur within a close temporal window. 
- **Systemic Demarcation**: Clustered findings are coalesced into a single `ResolvedFinding` with a composite title (e.g., "Cluster of: Garbage collection stall") and their confidence averaged out, keeping the UI clean and surfacing the systemic nature of the issue.

## Verification
- Added `riva_correlation_resolver_tests` to enforce behavior on clustering boundaries and ensure non-primary findings are safely preserved.
- Hooked the new tests into the `CTest` suite and validated the complete pass rate across all layers.
