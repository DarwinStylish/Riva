# Architecture

Riva is split between a standalone diagnostic core and Unreal Engine integration code.

The standalone core is built from `ue/Plugins/RivaEditor/Source/RivaCore` by CMake and contains the normalized trace model, analysis pipeline, signatures, resolvers, statistics, comparison, budget evaluation, and reporting. The Unreal Editor module owns TraceServices ingestion, Slate UI, file dialogs, and editor-thread coordination.

## Architecture guides

- [System overview and diagrams](system-overview.md)
- [Custom diagnostic signatures](custom-signatures.md)

## Decision records

- [ADR 0001: Keep the diagnostic core independent of Unreal Engine headers](adr/0001-pure-cpp20-diagnostic-core.md)
- [ADR 0002: Normalize all trace sources before analysis](adr/0002-normalized-trace-boundary.md)
- [ADR 0003: Rank evidence instead of claiming root-cause certainty](adr/0003-evidence-ranked-diagnostics.md)
- [ADR 0004: Keep TraceServices behind the Unreal integration boundary](adr/0004-traceservices-integration-boundary.md)

## Current verification boundary

The standalone C++20 build, CLI, deterministic JSON fixtures, regression gates, budget gates, and source-level plugin contracts are covered by automated tests.

The repository does not claim Unreal Engine package compatibility until `RunUAT BuildPlugin` succeeds against an installed UE 5.4 engine. Native `.utrace` ingestion currently normalizes frame boundaries and available timing-lane measurements. Native named-event extraction and Unreal Insights selection synchronization remain future work.
