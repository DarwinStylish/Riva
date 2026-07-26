# PR: Budgets Phase

**Feature Branch:** `feat/budgets`

## Summary
Introduced frame-time performance budgets. Budgets provide a hard constraint for diagnostic validation in both Unreal Editor workflows and continuous integration environments. 

## Key Changes
- **Core Engine:** Introduced `BudgetConfig` loaded from `budgets.json`. 
- **JSON Utility Abstraction:** Extracted JSON schema validation and primitive parsing into `json_utils.hpp` from the legacy monolithic trace loader to share logic cleanly.
- **CLI Gatekeeper:** Added `riva check-budget --budget <file> --trace <file>` command that parses a json trace, evaluates it against the budget, prints breached metrics, and exits with non-zero exit codes if any budget limit is exceeded. 
- **Unreal Editor Extension:** Integrated dynamic status bar updates in the Slate UI (`SRivaPanel`). If the trace violates any budget thresholds, the UI explicitly flags a red "Budget: BREACHED" warning, giving real-time feedback to developers.

## Automated Testing
- `budget_test.cpp` added to CTest harness, verifying schema loading logic. All 13 tests passed successfully.
