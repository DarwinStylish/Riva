# Release: Export Actions & Save Dialogs

## Overview
This release finalizes the export capabilities within the Unreal Engine plugin UI (`SRivaPanel`). Users can now export comprehensive Markdown and JSON diagnostic reports directly to their local disk using native desktop file dialogs, without interrupting their editor session. 

## Core Changes
- **Native OS Dialogs:** Integrated `FDesktopPlatformModule` to trigger native `SaveFileDialog` overlays when users click "Export Markdown" or "Export JSON".
- **Analysis Caching:** Implemented `GLastAnalysisResult` in `RivaTraceService.cpp` to statically hold the most recent `riva::AnalysisResult`. This bridges the gap between the stateless Slate UI and the stateful `riva::FReportEngine`, allowing delayed exportation.
- **Reporting Engine Hookup:** Connected the UI export dispatch commands directly to `riva::FReportEngine::GenerateMarkdownReport` and `GenerateJsonReport`.
- **Finding Iterator Fix:** Corrected the underlying `LoadAndAnalyze*` iterators to safely unpack `riva::ResolvedFinding` structures exposed by `Engine.Analyze()`.

## Verification
- Wrote `TestExportActions()` within `plugin_structure_test.cpp` to validate `DesktopPlatform` linkage and exportation API boundaries.
- Successfully passed the mandatory CMake test suite execution (`riva_plugin_structure_tests` and others).
