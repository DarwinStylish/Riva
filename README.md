# Riva

**Riva is an open-source performance diagnostics companion for Unreal Engine that turns trace telemetry into deterministic, evidence-ranked findings, regression comparisons, and enforceable performance gates.**

Riva complements Unreal Insights rather than replacing it. Unreal Insights remains the telemetry and timeline authority; Riva provides a local analysis layer for repeatable diagnostics, report generation, and build-pipeline automation.

## Status

Riva is an early-stage prototype. The standalone C++20 core, CLI, JSON trace workflow, regression and budget gates, deterministic fixtures, sanitizer configuration, parser fuzzing, and cross-platform standalone CI are implemented and testable without Unreal Engine.

The Unreal Editor integration targets UE 5.4 and includes a native Slate panel, asynchronous analysis, report export, and TraceServices-based `.utrace` ingestion. Native `.utrace` analysis currently normalizes frame boundaries and available timing-lane measurements. Named native event extraction is not implemented yet.

The repository does not claim UE 5.4 package compatibility until `RunUAT BuildPlugin` succeeds against a real UE 5.4 installation.

## What Riva does

Riva normalizes supported trace data into a common model and runs it through a deterministic diagnostics pipeline:

1. rolling-median spike detection;
2. UE-oriented diagnostic signatures;
3. evidence-relationship resolution;
4. confidence ranking;
5. multi-spike correlation;
6. statistics, performance scoring, budget evaluation, and reporting.

Built-in signatures currently cover:

- shader compilation stalls;
- pipeline state object misses;
- streaming and IO stalls;
- garbage collection stalls;
- Game Thread CPU spikes;
- Render Thread CPU spikes;
- RHI synchronization stalls;
- GPU variance associated with Lumen or Virtual Shadow Maps.

A primary finding is the highest-ranked supported diagnostic hypothesis in the implemented rule set. It is not presented as proof of causation. Reports retain evidence classifications and explicit confirmation guidance.

## Current capabilities

| Capability | Current state |
|---|---|
| Standalone C++20 diagnostic core | Implemented and covered by automated tests |
| JSON trace ingestion | Implemented |
| Markdown and JSON reports | Implemented |
| Baseline-versus-new trace comparison | Implemented |
| Performance budget gates | Implemented |
| Stable CLI gate exit codes | Implemented |
| Deterministic synthetic pathology fixtures | Implemented |
| Parser fuzz target with ASan/UBSan | Implemented |
| Unreal Editor Slate panel | Implemented in source |
| Asynchronous editor analysis and report export | Implemented in source |
| Native `.utrace` frame and timing-lane ingestion | Implemented, pending engine-level build validation |
| Native named-event extraction | Planned |
| Unreal Insights selection synchronization | Not implemented |
| UE 5.4 `BuildPlugin` package proof | Pending a real UE 5.4 installation |

## Architecture

Riva keeps diagnostic logic independent of Unreal Engine headers.

```text
Unreal Editor / TraceServices             Standalone CLI / CI
              |                                  |
              v                                  v
        source-specific ingestion and normalization
                         |
                         v
                  NormalizedTrace
                         |
                         v
        deterministic C++20 diagnostic core
                         |
            +------------+------------+
            |                         |
            v                         v
     findings/reports          comparison/budgets
```

The same core implementation under `ue/Plugins/RivaEditor/Source/RivaCore` is consumed by the Unreal plugin and built independently with CMake.

Architecture rationale is recorded in [docs/architecture](docs/architecture/index.md), including the normalized trace boundary, evidence-ranked diagnostics, and the TraceServices integration boundary.

## Build

Requirements:

- CMake 3.20 or newer;
- a C++20 compiler.

Configure, build, and test the strict release preset:

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

Sanitizer verification on a supported GCC or Clang environment:

```bash
cmake --preset sanitizers
cmake --build --preset sanitizers
ctest --preset sanitizers
```

See [Building and Packaging](docs/building.md) for configuration options, installation, and the separate Unreal Engine package-verification workflow.

## CLI

Analyze a trace:

```bash
./build/release/riva analyze samples/spike_shader_compile.json
```

Export JSON:

```bash
./build/release/riva analyze   samples/spike_shader_compile.json   --format json   --output report.json
```

Compare a baseline and a new trace:

```bash
./build/release/riva compare   samples/spike_cpu_game_thread.json   samples/spike_shader_compile.json
```

Apply a performance budget:

```bash
./build/release/riva check-budget   --budget samples/sample_budget.json   --trace samples/spike_shader_compile.json
```

Gate commands return exit code `3` when analysis succeeds but a regression or budget breach is detected. Operational failures and invalid command usage use separate exit codes documented in the [CLI guide](docs/user-guide/cli-usage.md).

## Worked examples and verification

The checked-in JSON traces are deterministic synthetic fixtures. They exercise known analysis branches and make regression behavior reproducible; they do not establish real-world diagnostic accuracy.

Start with:

- [Worked sample guide](docs/demo.md)
- [Verification boundary and reproducible evidence](docs/verification/README.md)
- [Unreal Editor plugin guide](docs/user-guide/unreal-plugin.md)
- [Trace comparison guide](docs/user-guide/trace-comparison.md)
- [Performance budgets guide](docs/user-guide/budgets-guide.md)

The verification documentation deliberately separates standalone CMake/CTest evidence from Unreal Engine package validation.

## Roadmap

Near-term work focuses on:

- real UE 5.4 `BuildPlugin` validation and packaging;
- native named-event extraction through TraceServices;
- a redistributable corpus of genuine Unreal traces;
- per-signature false-positive and false-negative evaluation;
- reusable CI integration examples;
- individually verified support for additional Unreal Engine versions.

See the full [roadmap](docs/roadmap.md).

## Contributing

Contributions should preserve Riva's core constraints: deterministic behavior, explicit evidence semantics, source-format isolation, and truthful verification boundaries.

See [CONTRIBUTING.md](CONTRIBUTING.md) before opening a pull request.

Security-sensitive issues should follow [SECURITY.md](SECURITY.md).

## License

Riva is licensed under the Apache License 2.0. See [LICENSE](LICENSE) and [NOTICE](NOTICE).
