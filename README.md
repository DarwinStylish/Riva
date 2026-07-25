# Riva

Riva is a deterministic performance diagnostics companion for Unreal Insights.

## Rules

- Pure C++20 core
- No Unreal headers in include/riva
- No manual .utrace parsing
- Deterministic analysis only

## Build

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Usage

Analyze a trace file and generate a diagnostic report in Markdown (default) or JSON format:

```bash
# Generate Markdown report to standard output
./build/riva analyze samples/spike_shader_compile.json

# Generate structured JSON report and save to file
./build/riva analyze samples/spike_shader_compile.json --format json --output report.json
```
