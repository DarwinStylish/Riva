# Building and Packaging Riva

## Supported standalone toolchains

Riva requires CMake 3.20 or newer and a C++20 compiler. CI builds the
standalone core, CLI, and tests with warnings treated as errors on Linux,
macOS, and Windows. A dedicated Linux Clang job also runs the Clang static
analyzer. Unreal Engine packaging is a separate verification step.

## Preset build

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

Available presets:

- `release`: optimized build, tests, CLI, and warnings as errors.
- `debug`: debug build with the same strict warnings.
- `sanitizers`: debug build with AddressSanitizer and UndefinedBehaviorSanitizer.

## Install the standalone artifacts

```bash
cmake --install build/release --prefix artifacts/install
```

This installs the CLI, standalone core library, and public C++ headers.

## Configuration options

| Option | Default | Purpose |
|---|---:|---|
| `RIVA_BUILD_TESTS` | `ON` | Build and register CTest coverage |
| `RIVA_BUILD_CLI` | `ON` | Build the `riva` executable |
| `RIVA_BUILD_FUZZERS` | `OFF` | Build Clang/libFuzzer targets |
| `RIVA_ENABLE_SANITIZERS` | `OFF` | Enable ASan and UBSan on GCC/Clang |
| `RIVA_WARNINGS_AS_ERRORS` | `OFF` | Make warnings in product, CLI, test, and fuzz targets fail the build |

## Unreal Engine 5.4

The standalone CMake build is not an Unreal package test. Use a source or
launcher UE 5.4 installation and run:

```bash
export UE_ROOT=/absolute/path/to/UnrealEngine-5.4
./scripts/verify-ue54-plugin.sh /absolute/path/to/RivaEditor-UE54
```

The script validates `Engine/Build/Build.version` before invoking
`RunUAT BuildPlugin`. Do not publish UE compatibility claims based only on the
plugin descriptor or the standalone CMake suite.

## Evidence bundle

See [`verification/README.md`](verification/README.md) for the worked examples
and the command that creates a checksum-backed reviewer evidence bundle.
