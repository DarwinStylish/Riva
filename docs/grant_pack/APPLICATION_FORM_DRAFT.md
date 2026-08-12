# Epic MegaGrants Official Application Form Draft

This document contains pre-filled responses formatted specifically for the official **Epic MegaGrants** online application portal fields (`unrealengine.com/megagrants`).

---

## 1. Project Information

### Project Name
**Riva — Deterministic Performance Diagnostics Companion for Unreal Engine**

### Primary Category
**Unreal Engine Tools & Open Source Components**

### Developer Portfolio & Website
`https://darwinstylish.dev`

### Repository URL
`https://github.com/DarwinStylish/Riva`

### Primary Contact Email
`darwin@darwinstylish.dev`

### License Type
**MIT License (Open Source)**

---

## 2. Short Project Description (100–150 Words)

Riva is an open-source, deterministic performance diagnostics companion for Unreal Engine. While Unreal Insights collects rich, multi-thread trace recordings, game developers often spend hours manually scrubbing timeline tracks to understand frame hitches. 

Riva automates bottleneck isolation by processing `.utrace` recordings and JSON trace exports through a 6-stage C++20 diagnostic pipeline. It isolates frame spikes, evaluates built-in signatures (GC, Shader Compilation, PSO Misses, Streaming IO, RHI Waits, Lumen/VSM GPU variance), graph-links cascading thread dependencies to elevate root causes over symptoms, and clusters repetitive spikes. 

Riva includes a native Unreal Editor Slate companion tab (`SRivaPanel`) with bi-directional Unreal Insights timeline selection sync, as well as a standalone headless CLI (`riva`) for automated performance budget gatekeeping and regression diffing in CI/CD build pipelines.

---

## 3. Detailed Technical Description & Architecture

Riva is designed around a strict decoupling model split into two primary components:

1. **`riva_core` (Pure C++20 Static Library)**:
   - Contains zero Unreal Engine header dependencies for maximum portability and instant compilation in headless build environments.
   - Converts raw trace inputs into a unified `NormalizedTrace` data model.
   - Runs diagnostic analysis through 6 sequential stages:
     `SpikeDetector` $\rightarrow$ `ISignature` Rules $\rightarrow$ `CausalChainResolver` $\rightarrow$ `ConfidenceResolver` $\rightarrow$ `CorrelationResolver` $\rightarrow$ `FReportEngine` / `ITraceComparator`.

2. **`RivaEditor` (Native Unreal Engine 5 Editor Plugin)**:
   - Implements `FRivaTraceService` to ingest native `.utrace` recordings via Unreal Engine's `TraceServices` module.
   - Provides a dockable Slate panel (`SRivaPanel`) under `Window -> Developer Tools -> Riva Performance Diagnostics`.
   - Executes trace analysis asynchronously on Unreal's background thread pool (`EAsyncExecution::ThreadPool`) to keep the editor UI 60 FPS responsive.
   - Links time range selections bi-directionally with Unreal Insights tracks.

3. **`riva` (Standalone CLI Executable)**:
   - Implements `riva analyze`, `riva compare` (regression diffing), and `riva check-budget` (CI gatekeeper returning exit code `3` on metric breaches).

---

## 4. Community & Developer Impact

Riva directly benefits the entire Unreal Engine ecosystem:

- **Indie Developers & Small Studios**: Eliminates the steep learning curve of manual timeline scrubbing, giving small teams AAA-grade performance diagnostic capabilities directly in the editor.
- **Enterprise & AAA Game Studios**: Integrates into automated PR pipelines to prevent "performance decay" during multi-year development cycles.
- **Open-Source Community**: Provides an extensible `ISignature` C++ framework for creating custom diagnostic rules (e.g. Niagara VFX, Chaos Physics, World Partitioning).

---

## 5. Current State & Proof of Concept

Riva is **not an unproven concept**—it is a working, production-grade codebase:
- **Developer Website**: [darwinstylish.dev](https://darwinstylish.dev)
- **Repository**: [GitHub - DarwinStylish/Riva](https://github.com/DarwinStylish/Riva)
- **Test Suite**: 13 CMake / CTest test executables passing with a 100% success rate.
- **Documentation**: Comprehensive documentation suite including User Guides, Architecture Specifications, and Mermaid diagrams.
- **Sample Datasets**: Includes 8 sample trace datasets covering Shader Compiles, PSO Misses, Streaming IO, GC, RHI Sync, and Lumen GPU variance.

---

## 6. Requested Funding Amount & High-Level Purpose

### Requested Amount
**$50,000 USD**

### Primary Purpose
To fund full-time development over a 6-month roadmap focused on expanding native `.utrace` event extraction (World Partitioning, Chaos Physics, Niagara), implementing automated call-stack symbolication (PDB/dSYM), building a standalone cross-platform GUI application, and packaging Riva as an officially distributed Marketplace & GitHub Action tool.
