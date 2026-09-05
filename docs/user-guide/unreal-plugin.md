# Unreal Editor Plugin User Guide (`RivaEditor`)

`RivaEditor` is a source plugin targeting Unreal Engine 5.4 on Win64 and Linux. It
adds a native, dockable Slate panel and runs Riva's deterministic C++ analysis
pipeline against JSON traces or real Unreal Insights `.utrace` recordings.

The descriptor, module boundaries, and standalone core are continuously checked,
but this repository does not claim UnrealBuildTool compatibility until
`BuildPlugin` succeeds against an installed UE 5.4 engine (see Package
verification below).

## Install in an Unreal Engine 5.4 project

1. Copy `ue/Plugins/RivaEditor` into the project's `Plugins` directory:

   ```text
   YourProject/
   └── Plugins/
       └── RivaEditor/
           ├── RivaEditor.uplugin
           └── Source/
   ```

2. Add the plugin to `YourProject.uproject`, or enable it in **Edit → Plugins**:

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

3. Regenerate project files, build the project's Editor target, and start the
   editor. This repository ships source, not precompiled engine binaries.

4. Open **Window → Developer Tools → Riva Performance Companion**.

## Analyze a trace

1. Select **Open Trace...** and choose a `.json` or `.utrace` file.
2. Select **Analyze**. Analysis runs on Unreal's background thread pool and UI
   updates return to the game thread. If a newer analysis starts or another trace
   is opened first, the stale result is discarded and cannot replace the current
   export state.
3. Select a finding to inspect its role, confidence, observed/derived evidence,
   suggested checks, and exact time window.
4. Use **Export Markdown...** or **Export JSON...** to preserve the report.

Normal parser rejection, missing trace channels, and corrupt files are shown as
errors; they are not converted into diagnostic findings.

## Capture a compatible native trace

The native loader needs game-frame boundaries. CPU/GPU timing channels provide
the thread attribution used by Riva's signatures. Launch the game or editor with
at least:

```text
-trace=frame,cpu,gpu -statnamedevents
```

Add channels such as `bookmark`, `file`, or `loadtime` when investigating those
systems. Stop the capture in Unreal Insights, then open the resulting binary
`.utrace` file in Riva. The repository intentionally does not ship a text file
masquerading as a trace fixture.

The UE 5.4 native path currently normalizes:

- game-frame start and duration;
- GameThread, RenderThread, and RHIThread top-level timing occupancy when present;
- the legacy GPU timing timeline when present; and
- trace thread identities.

Named CPU marker extraction is not implemented yet. JSON traces can carry named
events today; native `.utrace` analysis currently provides timing-based findings.

## Optional project budget

Place `RivaBudget.json` in the Unreal project's `Config` directory. For example:

```json
{
  "game_thread_ms_max": 16.667,
  "render_thread_ms_max": 16.667,
  "gpu_ms_max": 16.667,
  "duration_ms_max": 33.333
}
```

When the file is absent, the panel reports **Budget: Not configured**. An invalid
budget file fails the analysis with its parser error rather than silently
reporting that the budget passed.

## Current limitations

- Unreal Insights timeline selection synchronization is not implemented. The
  former logging-only prototype was removed rather than presented as integration.
- Native marker/bookmark names are not yet copied into `NormalizedTrace`.
- The plugin descriptor targets UE 5.4. Other engine versions require their own
  source build and compatibility verification.
- A real UE 5.4 Editor/package build is the final compatibility gate; standalone
  CMake tests cannot validate UnrealBuildTool or engine API linkage.

## Package verification

With `UE_ROOT` pointing at an Unreal Engine 5.4 installation, package the plugin
before distribution:

```bash
UE_ROOT=/path/to/UnrealEngine-5.4 ./scripts/verify-ue54-plugin.sh
```

The script rejects missing or non-5.4 engine roots before invoking `BuildPlugin`.
On Windows, run the equivalent `RunUAT.bat BuildPlugin` command with
`-TargetPlatforms=Win64`.
