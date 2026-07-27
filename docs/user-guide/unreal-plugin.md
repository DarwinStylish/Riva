# Unreal Editor Plugin User Guide (`RivaEditor`)

The **RivaEditor** plugin embeds Riva directly inside the Unreal Engine 5 Editor as a native, dockable Slate panel (`SRivaPanel`).

---

## Installation

1. Copy the `ue/Plugins/RivaEditor` directory into your project's `Plugins/` folder:
   ```
   YourProject/
   └── Plugins/
       └── RivaEditor/
           ├── RivaEditor.uplugin
           └── Source/
   ```
2. Enable the plugin in `YourProject.uproject`:
   ```json
   {
     "Plugins": [
       {
         "Name": "RivaEditor",
         "Enabled": true
       }
     ]
   }
   ```
3. Regenerate project files and compile your project in Unreal Editor.

---

## Opening the Companion Tab

In Unreal Editor, open the tab via the main menu:

```
Window ──> Developer Tools ──> Riva Performance Diagnostics
```

The tab spawns as a nomad dockable panel that can be docked anywhere alongside your viewport, Content Browser, or Unreal Insights layout.

---

## Panel Layout & Workflow

### 1. Minimal Toolbar
- **`Open Trace...`**: Select a trace recording (`.utrace` or `.json`) for analysis.
- **`Analyze`**: Run deterministic hitch analysis on the loaded trace.
- **`Export Markdown...`**: Save the diagnostic report to local disk via native OS save file dialog.
- **`Export JSON...`**: Save structured findings as JSON.
- **`Sync Insights`**: Toggle bidirectional selection linkage with Unreal Insights.
- **`Simulate Sync`**: Test time range selection synchronization in standalone editor sessions.

### 2. Split View Layout
- **Left Pane (Findings List - 35% Width)**: Displays detected hitches sorted by priority, displaying title, severity, role classification (`[Primary]` vs `[Secondary]`), calibrated confidence score, and time window.
- **Right Pane (Details & Evidence - 65% Width)**: Displays detailed diagnostic findings:
  - **Diagnostic Summary**: Title, severity badge, confidence percentage.
  - **Copy Actions**: `Copy Time Window` and `Copy Summary` buttons to copy payloads to the OS clipboard via `FPlatformApplicationMisc::ClipboardCopy`.
  - **Evidence Breakdown**: Table of evidence metrics (e.g. `gpu_ms`, `event` markers).
  - **Actionable Guidance**: "Suggested Next Steps" and "How to Confirm in Unreal Insights".

### 3. Dedicated Status Bar
- Bottom status bar displays real-time execution feedback and performance budget status (`Budget: OK` in green or `Budget: BREACHED` in red).

---

## Unreal Insights Bidirectional Selection Sync

When `Sync Insights` is enabled:
- **Outbound Sync**: Clicking a hitch row in `FindingsListView` broadcasts its timestamp window (`[StartTimeMs, EndTimeMs]`) to Unreal Insights to navigate the timing tracks automatically.
- **Inbound Sync**: Selecting a time range in Unreal Insights automatically highlights the corresponding hitch row in `FindingsListView` and updates the details pane.
