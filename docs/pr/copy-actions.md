# Release: Copy Actions & Time Window

## Overview
This release enhances the Riva Plugin Details Pane by providing direct copy-to-clipboard functionality and prominently displaying the active hitch time window. 

## Core Changes
- **Time Window Display**: Added `DetailsTimeWindowText` to surface the exact spike timestamps directly beneath the Diagnostic Title, improving immediate context visibility.
- **Copy Actions**: Introduced "Copy Time Window" and "Copy Summary" buttons directly into the Details Pane `SHorizontalBox`. 
- **Clipboard Integration**: Implemented event handlers `OnCopyTimeWindowClicked` and `OnCopySummaryClicked` utilizing Unreal Engine's native `FPlatformApplicationMisc::ClipboardCopy` for immediate diagnostic extraction.
- **State Tracking**: `SRivaPanel` now tracks `SelectedFinding` cleanly inside `OnFindingSelectionChanged` to pass the correct payload to the copy event handlers.

## Verification
- Embedded `TestCopyActions()` into `plugin_structure_test.cpp` to enforce structural dependencies on `PlatformApplicationMisc` and our Slate handlers. 
- Passed 100% of the CMake test suite (`riva_plugin_structure_tests`).
