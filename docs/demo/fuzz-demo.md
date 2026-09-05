# Riva Fuzz-Testing Demonstration

This document describes the native C++ fuzz-testing demonstration for Riva, designed to highlight the deterministic tracking and contextualization of software failures. 

## Purpose

The purpose of this optional demonstration is to show how an authentic parser
failure can be preserved as structured engineering evidence. It exercises the
same JSON parsing boundary used by the CLI. A clean fuzz run is a test result,
not proof that the parser is defect-free.

## Architecture

This demonstration uses:
- **C++20** with native Clang/LLVM libFuzzer (`-fsanitize=fuzzer`)
- **AddressSanitizer (ASan)** and **UndefinedBehaviorSanitizer (UBSan)**
- A self-contained orchestration script (`scripts/riva-fuzz-demo.sh`) for clean presentation.

## Selected Fuzz Target

The fuzz target directly exercises `riva::LoadNormalizedTraceFromJsonText` in `json_trace_loader.cpp`. 

### Why this target matters
This function is a critical input boundary. It consumes externally generated
JSON trace data and normalizes it into Riva's internal representation. Fuzzing
can discover parser defects and adds evidence about tested inputs; it cannot
prove resilience against every malformed input.

## Installation and Build

Required system dependencies:
```bash
sudo apt update
sudo apt install -y clang llvm cmake ninja-build build-essential pkg-config
```

Configure and build the fuzzing targets:
```bash
cmake -S . -B build-fuzz -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=RelWithDebInfo -DRIVA_BUILD_FUZZERS=ON
cmake --build build-fuzz --parallel
```

## Live Fuzzing

To run a live fuzzing session bounded to a standard time limit:
```bash
./scripts/riva-fuzz-demo.sh --live
```
This automatically invokes the `riva_fuzz_json_trace_loader` executable against the seed corpus located at `fuzz/corpus/json_trace_loader/`.

## Deterministic Replay

If libFuzzer discovers an authentic artifact (or if a known artifact is provided), you can reliably replay it:
```bash
./scripts/riva-fuzz-demo.sh --replay fuzz/artifacts/json_trace_loader/crash-<sha256>
```
The script reruns the exact artifact and creates an incident only when the
process exits non-zero and emits a recognized ASan or UBSan diagnostic.

## Evidence Files

Upon successful reproduction, the script builds a machine-readable evidence package at `.demo/incidents/<incident-id>/`. This includes:
- The exact crashing input
- The raw `libFuzzer` output (`fuzz.raw.log`)
- The reproduced execution logs and ASan traces (`reproduce.raw.log`, `stderr.log`)
- `metadata.json` (containing file hashes, source revisions, sizes, and timestamps)

## Recording Workflow

When recording the optional workflow:
1. Ensure the terminal is cleared and font size is roughly 18-22px.
2. Use the `--replay` option pointing to a genuine artifact if you want to explicitly demonstrate a crash sequence and evidence collection without relying on chance.
3. If `--live` is used, the script will gracefully report the parser's robustness if no crash is found.

## Limitations

- Native C++ Fuzzing requires Clang/LLVM toolchains, which may necessitate different handling on Windows environments. The provided CMake configuration is primarily intended for Linux/Ubuntu runners as requested.
- If the trace parser has no genuine defects, no crash will occur. We explicitly **do not** weaken production code to manufacture one.

## Diagnostic boundary

The current script preserves observed sanitizer evidence and source context. It
does not send crash data to an external model and does not invent a performance
diagnosis for malformed input that the normal trace pipeline cannot accept.

## Security Considerations

- The demo isolates all operations. Ensure `fuzz/artifacts` and `.demo` are listed in `.gitignore` if they contain sensitive or excessively large crash items.
- Ensure that no environment variables containing secrets are printed or captured during the orchestration or execution phases.
