# Roadmap

Riva is an early-stage open-source performance diagnostics companion for Unreal Engine. The roadmap prioritizes evidence quality and reproducibility before broader feature coverage.

## Current foundation

Implemented today:

- standalone C++20 diagnostic core;
- deterministic spike detection and eight built-in UE-oriented signatures;
- evidence classification, confidence ranking, causal relationship resolution, and multi-spike correlation;
- percentile statistics and performance scoring;
- baseline-versus-new trace comparison;
- performance budget evaluation;
- Markdown and JSON reporting;
- standalone CLI with stable gate exit codes;
- Unreal Editor Slate integration and asynchronous analysis workflow;
- native `.utrace` frame and timing-lane normalization through TraceServices;
- deterministic JSON fixtures, regression tests, sanitizer builds, parser fuzzing, and cross-platform standalone CI.

## Near-term priorities

1. Validate and package `RivaEditor` against a real Unreal Engine 5.4 installation using `RunUAT BuildPlugin`.
2. Extract supported named native events from TraceServices while preserving timestamps, thread identity, and provenance.
3. Build a redistributable corpus of genuine Unreal traces with independently reviewed labels.
4. Measure per-signature false-positive and false-negative behavior and publish known limits.
5. Add reusable CI integration examples for budget and regression gates.
6. Validate additional Unreal Engine versions individually rather than assuming compatibility.

## Later work

- stable signature-authoring SDK and compatibility guidance;
- deeper provider coverage where Unreal telemetry exposes reliable evidence;
- editor workflow improvements around evidence inspection;
- packaged releases and distribution workflows;
- optional integrations that preserve Riva local-first operation.

## Non-goals for the current prototype

Riva does not replace Unreal Insights, does not claim automatic root-cause certainty, and does not claim UE package compatibility without an engine-level build result.
