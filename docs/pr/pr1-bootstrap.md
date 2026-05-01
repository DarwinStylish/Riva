# PR1 - Bootstrap

## Summary

Bootstrap Riva with CMake, riva_core, CLI stub, and smoke tests.

## Verification

cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/riva version
./build/riva --help
