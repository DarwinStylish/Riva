# Performance Budgets Guide

Performance Budgets in Riva allow game teams to establish strict frame-time constraints across thread execution and total frame duration.


## JSON Schema Specification

Budget files are JSON objects containing one or more positive numeric threshold
limits specified in milliseconds (`ms`). An empty object is rejected so a CI
gate cannot pass vacuously.

```json
{
  "game_thread_ms_max": 16.6,
  "render_thread_ms_max": 16.6,
  "rhi_thread_ms_max": 16.6,
  "gpu_ms_max": 16.6,
  "duration_ms_max": 33.3
}
```

### Schema Fields

| Field Name | Type | Unit | Description |
|---|---|---|---|
| `game_thread_ms_max` | `number` (optional) | ms | Maximum allowed execution time for the Game Thread. |
| `render_thread_ms_max` | `number` (optional) | ms | Maximum allowed execution time for the Render Thread. |
| `rhi_thread_ms_max` | `number` (optional) | ms | Maximum allowed execution time for the RHI Thread. |
| `gpu_ms_max` | `number` (optional) | ms | Maximum allowed frame timing on the GPU. |
| `duration_ms_max` | `number` (optional) | ms | Maximum allowed overall frame duration. |


## Budget Presets

### 60 FPS Target (Standard Desktop / Console)

```json
{
  "game_thread_ms_max": 16.66,
  "render_thread_ms_max": 16.66,
  "rhi_thread_ms_max": 16.66,
  "gpu_ms_max": 16.66,
  "duration_ms_max": 16.66
}
```

### 30 FPS Target (Mobile / Lower-spec Target)

```json
{
  "game_thread_ms_max": 33.33,
  "render_thread_ms_max": 33.33,
  "rhi_thread_ms_max": 33.33,
  "gpu_ms_max": 33.33,
  "duration_ms_max": 33.33
}
```


## Relationship to Performance Score

Budget evaluation and performance scoring are separate in the current
prototype:

- Budget status compares every frame against the thresholds in the selected
  budget file.
- The 0-100 performance score uses `FScoreConfig` targets and trace statistics;
  it does not currently consume `BudgetConfig`.
- Missing subsystem telemetry is displayed as `N/A`, not as a passing score.


## Evaluating Budgets

### Via CLI Gatekeeper

Run `riva check-budget`:

```bash
./build/release/riva check-budget --budget config/budgets.json --trace traces/nightly_run.json
```

- If **all metrics pass**, the CLI prints `Budget check passed.` and exits with `0`.
- If **any metric breaches**, the CLI lists all breached metrics and exits with code `3`:
  ```
  Budget check failed! The following metrics exceeded their budget thresholds:
    - game_thread_ms
    - gpu_ms
  ```

### Via Unreal Editor Plugin

When a trace is loaded in the `SRivaPanel` companion tab, the status bar automatically evaluates active budget rules and displays:
- **Green status**: `Budget: OK`
- **Red warning**: `Budget: BREACHED (2 metrics)`
