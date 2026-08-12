# Changelog

All notable changes to **Riva** will be documented in this file.

The project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [0.2.0] - 2026-08-11

### Added

- **Quantitative Trace Comparison**: `FTraceStatistics` computes P50/P90/P95/P99 frame time percentiles, per-metric P95 (game thread, render thread, RHI, GPU, physics, AI, network, loading), hitch count, and hitch percentage. `FStatisticalComparison` and `FMetricDelta` enable metric-level diffing with configurable regression thresholds. Comparison reports now include a Markdown **Metric Summary** table with deltas, percentages, and regression indicators.
- **Synthetic Telemetry Generator**: `FTraceSynthesizer` programmatically generates `NormalizedTrace` instances with configurable pathology injections (shader compile, PSO miss, GC, streaming IO, RHI sync, CPU thread spikes, GPU variance) and known ground truth. Foundation for the performance pathology library.
- **Evidence Classification Taxonomy**: `EEvidenceClassification` (OBSERVED, DERIVED, CORRELATED, INFERRED, SUSPECTED, RECOMMENDED) on every `Evidence` item. Implements the Riva claim discipline.
- **First-Class Thread Entity**: `FTraceThread` and `EThreadType` for cross-thread correlation and utilization tracking.
- **Build Metadata**: `FBuildInfo` for regression tracking across builds, branches, platforms, and engine versions.
- **Scenario Metadata**: `FScenarioInfo` for scenario-based baselines.
- **Time-Series Counter Model**: `FTraceCounter` for memory, physics, and other domain signals.
- **Expanded Frame**: `physics_ms`, `ai_ms`, `network_ms`, `loading_ms`, `memory_bytes`, and per-frame `counters`.
- **Finding Context**: `affected_thread` and `affected_system` on every `Finding`.
- **JSON Parsing**: All new fields (build_info, scenario_info, threads, counters, expanded Frame). Fully backward-compatible.
- **Test Suite Expansion**: 15 CTest executables (up from 13).

### Changed

- **Breaking**: `ITraceComparator::Compare` signature now accepts `NormalizedTrace` references alongside `AnalysisResult` for quantitative metric-level comparison.
- All 8 builtin signatures now populate `affected_thread`, `affected_system`, and evidence classification.
- Reports render evidence classification tags, affected thread, and affected system.

### Removed

- Unused `third_party/rapidjson/` dependency.

---

## [0.1.0] - 2026-07-27

### Added

- **Core Diagnostics Engine**:
  - Implemented `RollingMedianSpikeDetector` for frame spike window detection.
  - Added built-in signatures for Garbage Collection (`STUT_GC`), Shader Compilation (`STUT_SHADER_COMPILE`), PSO Misses (`STUT_PSO_MISS`), Asset Streaming IO (`STUT_STREAMING_IO`), Game Thread CPU spikes (`STUT_CPU_GT`), Render Thread CPU spikes (`STUT_CPU_RT`), RHI Sync waits (`STUT_RHI_SYNC`), and Lumen/VSM GPU variance (`STUT_GPU_VARIANCE_LUMEN_VSM`).
  - Added `CausalChainResolver` for linking root causes and demoting symptomatic downstream stalls.
  - Added `ConfidenceResolver` for calibrated finding scoring.
  - Added `CorrelationResolver` for multi-spike clustering of repetitive findings.

- **Trace Comparison API**:
  - Added `ITraceComparator` and `DefaultTraceComparator` for diffing baseline vs. new trace runs.
  - Classifies diff findings into `regressions`, `improvements`, and `unchanged`.
  - Added `FReportEngine::GenerateComparisonReport` for Markdown comparison report formatting.

- **Performance Budgets**:
  - Added `BudgetConfig` JSON parsing with support for `game_thread_ms_max`, `render_thread_ms_max`, `rhi_thread_ms_max`, `gpu_ms_max`, and `duration_ms_max`.
  - Added CLI budget gatekeeper (`riva check-budget`).

- **CLI Application**:
  - Added `riva analyze` for single-trace Markdown/JSON report generation.
  - Added `riva compare` for trace regression diffing.
  - Added `riva check-budget` for automated CI performance gatekeeping.
  - Added `riva version` for version reporting.

- **Unreal Editor Plugin (`RivaEditor`)**:
  - Added native Slate panel `SRivaPanel` with dockable tab spawner (`RivaEditorTab`).
  - Added `FRivaTraceService` supporting `.utrace` (Unreal Insights TraceServices) and `.json` trace ingestion.
  - Added non-blocking async analysis execution on Unreal's background thread pool.
  - Added bidirectional Unreal Insights selection synchronization.
  - Added clipboard copy actions (`Copy Summary`, `Copy Time Window`) and native OS save file dialogs for Markdown and JSON exports.

- **Test Suite**:
  - Added 13 CMake/CTest executables covering core engine, signatures, resolvers, budget parser, trace comparator, CLI integration, report engine, and plugin structure verification.
