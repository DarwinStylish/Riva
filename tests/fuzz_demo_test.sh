#!/usr/bin/env bash
set -euo pipefail

echo "Running tests for scripts/riva-fuzz-demo.sh"

DEMO_SCRIPT="./scripts/riva-fuzz-demo.sh"
if ! bash -n "$DEMO_SCRIPT"; then
    echo "FAILED: demo script has invalid shell syntax."
    exit 1
fi

if ! grep -q 'REPRO_EXIT -ne 0 && -n "$SANITIZER"' "$DEMO_SCRIPT"; then
    echo "FAILED: incident creation is not gated on process failure plus sanitizer evidence."
    exit 1
fi

if ! grep -q 'No incident bundle was created' "$DEMO_SCRIPT"; then
    echo "FAILED: failed reproduction does not explicitly reject evidence creation."
    exit 1
fi

if grep -q '"reproducible": true' "$DEMO_SCRIPT" &&
   ! grep -q 'Replay did not reproduce a sanitizer-confirmed failure' "$DEMO_SCRIPT"; then
    echo "FAILED: reproducibility metadata is not protected by a fail-closed path."
    exit 1
fi

echo ""
echo "Fuzz demonstration integrity checks passed."
