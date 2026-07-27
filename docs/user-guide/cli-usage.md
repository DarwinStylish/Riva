# CLI Usage & CI/CD Integration Guide

The Riva Command Line Interface (`riva`) is a standalone C++20 executable designed for automated trace analysis, performance budget gatekeeping, and regression testing in Continuous Integration (CI/CD) pipelines.

---

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
./build/riva analyze samples/spike_shader_compile.json

# Save JSON report to file
./build/riva analyze samples/spike_shader_compile.json --format json --output report.json
```

---

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
./build/riva compare samples/spike_shader_compile.json samples/spike_pso_miss.json --output comparison.md
```

---

### 3. `riva check-budget`

Evaluates a trace against a budget configuration file (`budgets.json`) and returns a non-zero exit code if any metric exceeds its threshold.

```bash
riva check-budget --budget <budget_file> --trace <trace_file>
```

#### Example

```bash
./build/riva check-budget --budget samples/sample_budget.json --trace samples/spike_cpu_game_thread.json
```

---

### 4. `riva version`

Prints product name and version number.

```bash
./build/riva version
# Output: Riva 0.1.0
```

---

## Exit Codes Reference

| Exit Code | Meaning | Usage Scenario |
|---|---|---|
| `0` | **Success** | Analysis finished successfully; or budget check passed without breaches. |
| `1` | **Runtime / I/O Error** | Missing trace file, invalid JSON syntax, or internal loading failure. |
| `2` | **Invalid Command / Usage** | Unknown CLI flag, missing mandatory parameters, or `--help` requested. |
| `3` | **Budget Breached** | `check-budget` completed and detected one or more exceeded metric thresholds. |

---

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
      - uses: actions/checkout@v3

      - name: Build Riva CLI
        run: |
          cmake -S . -B build
          cmake --build build --target riva -j

      - name: Run Budget Check
        run: |
          ./build/riva check-budget --budget config/budgets.json --trace test_runs/latest.json

      - name: Run Regression Diffing
        if: always()
        run: |
          ./build/riva compare test_runs/baseline.json test_runs/latest.json --output pr_diff.md
```
