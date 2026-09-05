# ADR 0001: Keep the diagnostic core independent of Unreal Engine headers

Status: Accepted

Date: 2026-09-05

## Context

Riva needs to support two execution environments: an Unreal Editor workflow and standalone command-line or CI workflows. Coupling diagnostic algorithms directly to Unreal types would make headless builds slower, increase engine-version coupling, and prevent the same analysis code from being tested independently.

## Decision

The diagnostic implementation remains standard C++20 and does not depend on Unreal Engine headers.

The source is located under `ue/Plugins/RivaEditor/Source/RivaCore` so the same implementation can be consumed as an Unreal module and built by standalone CMake. Unreal-specific behavior belongs in `RivaEditor`.

`RivaCoreModule.cpp` is the module bootstrap exception. It exists to register the module with Unreal but is not part of the standalone diagnostic algorithm surface.

## Consequences

The core can be built and tested on Linux, macOS, and Windows without Unreal Engine installed.

The CLI and CI gates use the same analysis implementation as the editor integration.

Unreal data must cross an explicit translation boundary before analysis.

Engine-specific features cannot leak into core APIs merely for convenience.
