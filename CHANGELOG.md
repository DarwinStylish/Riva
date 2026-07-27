# Changelog

All notable changes to **Riva** will be documented in this file.

The project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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
