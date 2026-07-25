# Feature Release: RivaEditor Unreal Engine Plugin Shell (`feat/editor-plugin-shell`)

## Overview

This release establishes the foundational Unreal Engine 5 Editor plugin shell (`RivaEditor`) for our deterministic performance diagnostics companion. With this architecture in place, Unreal Engine projects can embed Riva as an editor module and access the dockable diagnostic companion tab directly within the Unreal Editor interface.

## Key Accomplishments

### 1. Unreal Engine Plugin Architecture (`ue/Plugins/RivaEditor/`)
- Implemented the plugin descriptor `RivaEditor.uplugin`, configuring the module for `Editor` type and `Default` loading phase under the `Performance` category.
- Established Unreal Build Tool (UBT) module rules in `RivaEditor.Build.cs`, defining dependencies on `Slate`, `SlateCore`, `ToolMenus`, `WorkspaceMenuStructure`, `EditorStyle`, and establishing an include path to the RivaCore header directory.

### 2. Editor Module & Lifecycle Management
- Created `FRivaEditorModule` (`RivaEditorModule.h` and `RivaEditorModule.cpp`) implementing `IModuleInterface`.
- Registered a nomad dockable tab spawner (`RivaEditorTab`) assigned to the Editor Developer Tools menu category (`FWorkspaceItem::GetDeveloperTools()`).
- Extended the Level Editor Window menu (`LevelEditor.MainMenu.Window`) with a menu entry to invoke the companion tab on demand.
- Applied standard Unreal Engine localization namespaces (`LOCTEXT_NAMESPACE` and `LOCTEXT`) across all UI strings and labels.

### 3. Dockable Slate UI Companion Tab
- Created the compound Slate widget `SRivaPanel` (`SRivaPanel.h` and `SRivaPanel.cpp`).
- Constructed a clean, minimal vertical box layout featuring a styled header banner and status messaging tailored for Unreal Engine Editor dark mode aesthetics.
- Declared the logging category `LogRivaEditor` for engine diagnostic logging.

### 4. Automated Plugin Structure Verification
- Added `tests/plugin_structure_test.cpp` and registered `riva_plugin_structure_tests` in our CMake build suite. This test validates the schema of `RivaEditor.uplugin`, verifies UBT module dependencies, and asserts the presence and structure of all required Unreal Engine plugin C++ files.

## Verification

The complete build and test suite has been executed across all target suites:
```bash
cmake -S . -B build && cmake --build build -j && ctest --test-dir build --output-on-failure
```
All test suites pass with a 100% success rate across 9 target test suites.
