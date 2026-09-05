# CLI Usage & CI/CD Integration Guide

The Riva Command Line Interface (`riva`) is a standalone C++20 executable designed for automated trace analysis, performance budget gatekeeping, and regression testing in Continuous Integration (CI/CD) pipelines.


## Command Reference

### 1. `riva analyze`

Analyzes a single trace file (`.json`) and generates a diagnostic report in Markdown or JSON.

```bash
riva analyze <trace_file> [options]
```

#### Options

| Option | Description | Default |
|---|---|---|
| `--format, -f <markdown\|json>` | Output report format | `markdown` |
| `--output, -o <file_path>` | Write report to file instead of standard output | `stdout` |

#### Examples

```bash
# Output Markdown report to stdout
./build/release/riva analyze samples/spike_shader_compile.json

# Example Output Snippet:
# ## Performance Summary
# - **Overall Score**: 38 / 100 (F)
# - **P50**: 16.20 ms | **P95**: 44.82 ms | **P99**: 47.36 ms
# - **Hitch Rate**: 33.3% (1 of 3 frames)


# Save JSON report to file (includes 'statistics' and 'performance_score' blocks)
./build/release/riva analyze samples/spike_shader_compile.json --format json --output report.json
```

The JSON loader rejects inputs larger than 64 MiB and documents nested more
than 128 levels deep. These defensive defaults bound memory and stack use for
untrusted CI artifacts; library callers can choose stricter limits through
`JsonTraceLoaderOptions`.


### 2. `riva compare`

Diffs a baseline trace against a new trace run to identify regressions, resolved issues, and persistent findings.

```bash
riva compare <baseline_trace> <new_trace> [options]
```

#### Options

| Option | Description | Default |
|---|---|---|
| `--output, -o <file_path>` | Write comparison report to file instead of standard output | `stdout` |

#### Example

```bash
./build/release/riva compare samples/spike_shader_compile.json samples/spike_pso_miss.json --output comparison.md
```


### 3. `riva check-budget`

Evaluates a trace against a budget configuration file (`budgets.json`) and returns a non-zero exit code if any metric exceeds its threshold.

```bash
riva check-budget --budget <budget_file> --trace <trace_file>
```

#### Example

```bash
./build/release/riva check-budget --budget samples/sample_budget.json --trace samples/spike_cpu_game_thread.json
```


### 4. `riva version`

Prints product name and version number.

```bash
./build/release/riva version
# Output: Riva 0.2.0
```


## Exit Codes Reference

| Exit Code | Meaning | Usage Scenario |
|---|---|---|
| `0` | **Success** | Command completed and no comparison or budget gate failed. |
| `1` | **Runtime / I/O Error** | Missing trace file, invalid JSON syntax, or internal loading failure. |
| `2` | **Invalid Command / Usage** | Unknown CLI flag or missing mandatory parameters. |
| `3` | **Gate Failed** | `compare` detected a regression or `check-budget` detected a breach. |


## Continuous Integration Setup

### GitHub Actions Workflow Example

```yaml
name: Performance Gatekeeper

on:
  pull_request:
    branches: [ main ]

jobs:
  performance-check:
    runs-on: ubuntu-latest

    steps:
      - uses: actions/checkout@v4

      - name: Build Riva CLI
        run: |
          cmake --preset release
          cmake --build --preset release --target riva

      - name: Run Budget Check
        run: |
          ./build/release/riva check-budget --budget config/budgets.json --trace test_runs/latest.json

      - name: Run Regression Diffing
        if: always()
        run: |
          ./build/release/riva compare test_runs/baseline.json test_runs/latest.json --output pr_diff.md
```
