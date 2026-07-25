# CLI Trace Analysis & Reporting Pipeline

## Summary

Upgrades the Riva command-line diagnostic tool (`riva-cli`) with full trace analysis and reporting orchestration, connecting ingestion, stall detection, and report generation.

## Included

- Upgraded CLI entry point (`apps/riva-cli/main.cpp`) with argument parsing and subcommand handling
- Support for `analyze` command: `riva analyze <trace_file> [options]`
- Output format selection via `--format markdown` (default) and `--format json`
- File export support via `--output <file_path>` (defaults to stdout)
- Automatic JSON trace ingestion via integrated `TraceAdapterRegistry` and `JsonTraceAdapter`
- End-to-end integration test suite verifying diagnostic pipeline execution against sample trace data (`tests/cli_integration_test.cpp`)

## Verification

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/riva analyze samples/spike_shader_compile.json --format markdown
./build/riva analyze samples/spike_shader_compile.json --format json
```
