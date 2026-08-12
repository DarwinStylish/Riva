# Project Summary: Riva

**Riva** is an open-source, deterministic performance diagnostics companion for Unreal Engine. It bridges the gap between raw data collection in Unreal Insights and actionable bottleneck resolution for game developers and CI/CD automation pipelines.

---

## The Problem & Solution

Unreal Insights records millions of runtime trace events per minute, but game teams face severe manual overhead scrubbing timelines, sorting through cascading thread symptoms, and catching regressions late in development. 

Riva shifts optimization left by providing automated, deterministic diagnostics:
- **Deterministic Spike Detection**: Isolates hitch frames against a rolling median window.
- **Causal Graph Resolution**: Links concurrent stalls, elevating root causes (`STUT_STREAMING_IO`, `STUT_GC`) over symptomatic downstream waits (`STUT_RHI_SYNC`, `STUT_CPU_GT`).
- **Multi-Spike Correlation**: Clusters repetitive temporal hitches into clean composite findings.
- **Unreal Editor Slate Panel (`SRivaPanel`)**: Dockable tab with bi-directional Unreal Insights timeline selection sync, clipboard actions, and report export dialogs.
- **Headless CI/CD Gatekeeper (`riva`)**: Pure C++20 CLI with zero Unreal Engine header dependencies for instant budget checks (`riva check-budget`) and regression diffing (`riva compare`).

---

## Technical Comparison Matrix

| Capability | Raw Profilers *(Unreal Insights, PIX)* | Cloud APM *(Sentry, Datadog)* | **Riva Diagnostics** |
|---|---|---|---|
| **Primary Output** | Raw timeline tracks & millions of events | Stack traces & cloud web dashboards | **Actionable, root-cause Markdown/JSON reports** |
| **Diagnostic Effort** | Manual timeline scrubbing by developers | Aggregated telemetry graphs | **100% automated spike detection & bottleneck classification** |
| **Symptom Filtering** | Shows all thread stalls equally | Shows raw call stacks | **Causal Graphing**: Elevates root causes (IO/GC) over symptom waits (RHI Sync) |
| **Repetitive Spikes** | Spams 50+ identical timeline markers | Aggregates event counts | **Multi-Spike Correlation**: Clusters repetitive hitches into composite findings |
| **UE Subsystem Context** | Raw markers (`GarbageCollect`, `ShaderCompile`) | Generic CPU/GPU execution time | **Built-in UE5 Signatures**: Understands PSO misses, Zen IO, Lumen/VSM GPU passes |
| **In-Editor UI Sync** | Separate external window | Web browser overlay | **Native Dockable Slate Tab** with bi-directional Insights selection linkage |
| **CI/CD Regression Diffing** | None (requires human inspection) | Post-release cloud analytics | **Headless CLI (`riva compare`)** for automated PR gatekeeping in 5ms |
| **Infrastructure** | Local | Required Cloud Accounts & SaaS Fees | **100% Local & Offline**, zero accounts, zero cloud dependencies |

---

## Key Advantages

Riva is **100% open source (MIT License)**, production-ready with a **100% passing 13-executable CTest suite**, and runs **100% locally with zero cloud or account friction**. Its decoupled C++20 core architecture ensures maximum stability across Unreal Engine version updates.
