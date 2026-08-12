# Milestones & Budget Request Breakdown

**Grant Project**: Riva — Deterministic Performance Diagnostics Companion for Unreal Engine  
**Requested Amount**: **$50,000 USD**  
**Duration**: 6 Months (4 Technical Milestones)

---

## Technical Milestones Roadmap

```
Month 1-2                 Month 3                   Month 4-5                 Month 6
┌──────────────────────┐  ┌──────────────────────┐  ┌──────────────────────┐  ┌──────────────────────┐
│  Milestone 1         │  │  Milestone 2         │  │  Milestone 3         │  │  Milestone 4         │
│  Extended .utrace    │  │  Call-stack          │  │  Standalone Desktop  │  │  Fab Marketplace     │
│  Subsystem Extraction│  │  Symbolication       │  │  GUI Application     │  │  & GitHub Actions    │
│  ($15,000)           │  │  ($15,000)           │  │  ($10,000)           │  │  ($10,000)           │
└──────────────────────┘  └──────────────────────┘  └──────────────────────┘  └──────────────────────┘
```

---

### Milestone 1: Extended Subsystem Extraction & Advanced UE5 Signatures
* **Duration**: Months 1 – 2
* **Budget**: **$15,000 USD**
* **Deliverables**:
  - Expand `FRivaTraceService` to extract deep TraceServices markers for **World Partitioning** grid loading, **Chaos Physics** solver steps, and **Niagara VFX** compilation lanes.
  - Implement 5 new built-in core signatures (`STUT_WORLD_PARTITION`, `STUT_CHAOS_PHYSICS`, `STUT_NIAGARA_COMPILE`, `STUT_ANIMATION_EVAL`, `STUT_TEXTURE_STREAMING`).
  - Add CTest suite verification for all new subsystem signatures.

---

### Milestone 2: Call-Stack Symbolication & Native Stack Unwinding
* **Duration**: Month 3
* **Budget**: **$15,000 USD**
* **Deliverables**:
  - Integrate native symbol resolution for call-stacks embedded in `.utrace` recordings (PDB on Windows, dSYM on macOS, ELF/DWARF on Linux).
  - Surface function-level flamegraph evidence directly inside the `SRivaPanel` Slate details pane and Markdown reports.
  - Ensure zero external cloud dependencies during offline symbol resolving.

---

### Milestone 3: Cross-Platform Standalone Desktop Companion App
* **Duration**: Months 4 – 5
* **Budget**: **$10,000 USD**
* **Deliverables**:
  - Develop a standalone desktop GUI companion application (`riva-gui`) using C++20 and Dear ImGui / Slate for developers who want to inspect traces outside of Unreal Editor.
  - Support drag-and-drop trace file loading, live trace comparison side-by-side split view, and direct export.
  - Distribute pre-built native binaries for Windows, macOS, and Linux.

---

### Milestone 4: Fab Marketplace Integration & CI/CD Tooling Package
* **Duration**: Month 6
* **Budget**: **$10,000 USD**
* **Deliverables**:
  - Package `RivaEditor` for official distribution on Epic Games' **Fab Marketplace**.
  - Publish an official GitHub Action (`riva-action`) and Unreal Automation Tool (UAT) plugin script for zero-config CI/CD pipeline integration.
  - Create video walkthroughs and comprehensive developer documentation.

---

## Summary of Funds Allocation

| Expense Category | Allocation | Percentage | Description |
|---|---|---|---|
| **Core C++ Development** | $30,000 | 60% | Full-time engineering for TraceServices extraction, symbolication, and signature algorithms. |
| **GUI & Editor UX Design** | $10,000 | 20% | Standalone desktop application development & Slate UI refinements. |
| **CI/CD Tooling & Distribution** | $10,000 | 20% | GitHub Action creation, Fab Marketplace packaging, video tutorials, and documentation. |
| **Total** | **$50,000** | **100%** | |
