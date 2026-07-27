# Contributing to Riva

Thank you for your interest in contributing to Riva! This document outlines our development principles, coding standards, and pull request workflow.

---

## Core Rules

1. **Pure C++20 Core**: All code in `include/riva/` and `src/` must conform strictly to C++20 standards.
2. **Zero Unreal Headers in Core**: The core library (`riva_core`) must NEVER include Unreal Engine headers (`CoreMinimal.h`, `UObject/`, etc.). All Unreal Engine integration is isolated inside `ue/Plugins/RivaEditor/`.
3. **Deterministic Diagnostics**: Analysis algorithms must be completely deterministic. Avoid non-reproducible random heuristics or wall-clock timing dependencies.
4. **Clean Error Handling**: Use `riva::Status` and `riva::StatusCode` for error reporting rather than raw C++ exceptions.

---

## Coding Standards

### Core C++ (`include/riva/`, `src/`)

- Use `std::size_t`, `std::uint64_t`, `std::string_view`, and `std::optional` explicitly.
- Namespace all symbols inside `namespace riva`.
- Use `snake_case` for local variables and function parameters.
- Use `PascalCase` for type names and class names.
- Mark non-mutating methods with `const` and getters with `[[nodiscard]]`.

### Unreal Engine Plugin (`ue/Plugins/RivaEditor/`)

Follow **Epic Games' Unreal Engine C++ Coding Conventions**:
- Use Unreal type prefixes:
  - `F` for structs and non-UObject classes (`FReportEngine`, `FRivaUiFinding`).
  - `E` for enums (`EReportFormat`).
  - `I` for interfaces (`ITraceComparator`).
  - `b` prefix for booleans (`bBreached`, `bSyncWithInsightsEnabled`).
- Use Unreal parameter prefixes:
  - `In` for input parameters (`InResult`, `InOptions`).
  - `Out` for output parameters (`OutMarkdown`, `OutReport`).
- Apply `LOCTEXT_NAMESPACE` and `LOCTEXT` macro wrappers for all user-facing Slate strings.

---

## Building and Running Tests Locally

Always verify that the full build and CTest suite pass before submitting a pull request:

```bash
# Configure build
cmake -S . -B build

# Build target binaries
cmake --build build -j

# Execute full CTest suite
ctest --test-dir build --output-on-failure
```

All 13 test executables must pass with 100% success.

---

## Submitting Pull Requests

1. Fork the repository and create a feature branch (`feat/your-feature-name`).
2. Keep commits atomic and descriptive.
3. Include unit tests in `tests/` for any new signatures, resolvers, or CLI options.
4. Open a Pull Request against the `main` branch.
