# ADR 0004: Keep TraceServices behind the Unreal integration boundary

Status: Accepted

Date: 2026-09-05

## Context

Native Unreal Insights recordings require Unreal Engine TraceServices APIs. Those APIs are engine-specific and may vary between engine releases.

The standalone core must remain usable without an Unreal installation, while the editor plugin still needs access to native trace providers and editor services.

## Decision

TraceServices access is owned by `FRivaTraceService` in the `RivaEditor` module.

The service opens supported `.utrace` recordings, reads the providers Riva currently understands, converts their data into `riva::NormalizedTrace`, and then invokes the same core analysis pipeline used by standalone workflows.

The repository treats Unreal package verification separately from standalone CMake verification.

## Consequences

Engine API changes are isolated to the Unreal integration layer.

Standalone CI cannot prove that the plugin compiles, packages, or loads under Unreal Engine.

A successful `RunUAT BuildPlugin` result against the target engine version is required before package compatibility is claimed.

The current native path does not extract named TraceServices marker events and does not synchronize Riva selections with the Unreal Insights timing view. Those capabilities remain explicit future work.
