# Performance Budgets Guide

Performance Budgets in Riva allow game teams to establish strict frame-time constraints across thread execution and total frame duration.

---

## JSON Schema Specification

Budget files are JSON files containing optional numeric threshold limits specified in milliseconds (`ms`):

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

---

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

}
```

---

## Impact on Performance Score

Budgets are now tightly integrated into Riva's **Performance Scoring System** (the 0-100 A-F grade on every report). 

- **Subsystem Deductions**: If a subsystem (e.g., Game Thread) exceeds 80% of its budget allowance in P95 calculations, its individual score incurs penalties. If it exceeds 100% of the budget, it takes massive penalty deductions, dropping its grade to C, D, or F.
- **Hitch Penalties**: If the trace's hitch percentage exceeds the acceptable allowance (default 2%), the overall trace score drops rapidly.

---

## Evaluating Budgets

### Via CLI Gatekeeper

Run `riva check-budget`:

```bash
./build/riva check-budget --budget config/budgets.json --trace traces/nightly_run.json
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
