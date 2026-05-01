# PR3 - Builtin Signatures

## Summary

Adds the first deterministic builtin Riva signature set.

## Included

- STUT_SHADER_COMPILE
- STUT_PSO_MISS
- STUT_STREAMING_IO
- STUT_CPU_GT
- STUT_CPU_RT
- STUT_RHI_SYNC
- STUT_GC
- STUT_GPU_VARIANCE_LUMEN_VSM

## Architecture Discipline

- Pure C++20
- No Unreal headers
- Marker and timing based deterministic evidence only
- Conservative confidence
- Every finding includes suggested next steps
- Every finding includes how-to-confirm guidance

## Verification

cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/riva version
./build/riva --help
