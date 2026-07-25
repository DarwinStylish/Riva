# Report Engine Implementation

## Summary

Adds deterministic Markdown and JSON report generation for Unreal Engine performance diagnostic analysis results.

## Included

- Report Engine public API (`include/riva/report_engine.hpp`)
- Deterministic Markdown report formatter with Executive Summary, Findings List, Evidence Breakdown, and Actionable Guidance
- Structured JSON report generator matching Riva diagnostic schema
- Report options configuration (`FReportOptions`) for customizing report sections and formatting
- Support for both one-shot reporting and targeted format generation (`EReportFormat`)
- Comprehensive unit test suite covering Markdown formatting, JSON structure, toggles, and edge cases (`tests/report_engine_test.cpp`)

## Verification

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```
