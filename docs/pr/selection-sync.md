# Feature Release: Bidirectional Unreal Insights Selection Sync (`feat/selection-sync`)

## Overview

This release enables bidirectional time range selection synchronization between the Slate companion tab (`SRivaPanel`) and Unreal Insights. When a developer selects a detected performance hitch in the companion tab findings list, the corresponding numeric time window (`[StartTimeMs, EndTimeMs]`) is broadcast to Unreal Insights. Conversely, selecting a time window in Unreal Insights automatically highlights overlapping hitches in the companion tab.

## Key Accomplishments

### 1. Numeric Time Range Representation and Service Boundary
- Added numeric timestamps (`double StartTimeMs`, `double EndTimeMs`) to `FRivaUiFinding`, allowing exact timestamp comparisons without string parsing.
- Implemented static synchronization methods in `FRivaTraceService` (`BroadcastTimeRangeSelection`, `RegisterInsightsSelectionCallback`, `UnregisterInsightsSelectionCallback`, `SimulateInsightsSelection`), isolating all session timing and navigation interaction behind our service boundary.
- Populated numeric start and end timestamps automatically during both `.utrace` and `.json` trace analysis in `FRivaTraceService`.

### 2. Bidirectional UI Synchronization & Toolbar Controls
- Added an `SCheckBox` toggle (`Sync Insights`) to the Slate toolbar to dynamically enable or disable synchronization.
- Added an `SButton` trigger (`Simulate Sync`) to the Slate toolbar, enabling deterministic simulation of inbound Unreal Insights time range selections.
- Implemented outbound synchronization: selecting an item in `FindingsListView` broadcasts its timing range when sync is enabled.
- Implemented inbound synchronization: receiving a time range selection iterates over loaded findings and automatically selects overlapping hitches in `FindingsListView`, updating the status bar and details pane.

### 3. Automated Structure and Synchronization Verification
- Expanded `tests/plugin_structure_test.cpp` with `TestSelectionSync()`, verifying timestamp declarations, callback registration, simulation triggers, and widget synchronization handlers.

## Verification

The complete build and test suite has been executed across all target suites:
```bash
cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure
```
All test suites pass with a 100% success rate across 9 target test suites.
