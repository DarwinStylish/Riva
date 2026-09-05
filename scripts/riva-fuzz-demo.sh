#!/usr/bin/env bash
set -euo pipefail

# Semantic Colors
RESET="\e[0m"
WHITE="\e[1;37m"
GREEN="\e[1;32m"
RED="\e[1;31m"
CYAN="\e[1;36m"
YELLOW="\e[1;33m"

# State
MODE=""
ARTIFACT_PATH=""
TARGET="riva_fuzz_json_trace_loader"
BUILD_DIR="build-fuzz"
EXECUTABLE="./${BUILD_DIR}/${TARGET}"
DEMO_DIR=".demo"
LOG_DIR="${DEMO_DIR}/logs"
INCIDENT_DIR="${DEMO_DIR}/incidents"
WORK_CORPUS=""

cleanup() {
    case "$WORK_CORPUS" in
        /tmp/riva-fuzz-corpus.*) rm -rf -- "$WORK_CORPUS" ;;
    esac
}
trap cleanup EXIT

function usage() {
    echo "Usage:"
    echo "  $0 --live"
    echo "  $0 --replay <artifact_path>"
    exit 1
}

# Parse Args
if [[ "${1:-}" == "--live" ]]; then
    MODE="live"
elif [[ "${1:-}" == "--replay" ]]; then
    MODE="replay"
    if [[ -z "${2:-}" ]]; then
        echo -e "${RED}× Error: --replay requires an artifact path${RESET}"
        usage
    fi
    ARTIFACT_PATH="$2"
else
    usage
fi

mkdir -p "$LOG_DIR"
mkdir -p "$INCIDENT_DIR"
mkdir -p fuzz/artifacts/json_trace_loader

function print_header() {
    echo -e "┌─────────────────────────────────────────────────────────────┐"
    echo -e "│ ${WHITE}RIVA${RESET}                                                        │"
    echo -e "│ Engineering Failure Intelligence                            │"
    echo -e "└─────────────────────────────────────────────────────────────┘"
    echo ""
}

function print_footer() {
    echo -e "────────────────────────────────────────────────────────────"
    echo ""
    echo -e "${WHITE}CURRENT RIVA PROTOTYPE${RESET}"
    echo ""
    echo -e "    Unreal / trace telemetry"
    echo -e "            ↓"
    echo -e "    normalization + diagnostics"
    echo ""
    echo -e "${CYAN}VERIFIABLE FAILURE EVIDENCE${RESET}"
    echo ""
    echo -e "    exact input + sanitizer output"
    echo -e "                 ↓"
    echo -e "    source revision + target context"
    echo -e "                 ↓"
    echo -e "       reviewable incident bundle"
    echo ""
    echo -e "The script records only evidence observed during this run."
    echo -e "It does not generate a diagnosis for unsupported crash artifacts."
    echo ""
    echo -e "┌─────────────────────────────────────────────────────────────┐"
    echo -e "│                                                             │"
    echo -e "│                         ${WHITE}RIVA${RESET}                                │"
    echo -e "│                                                             │"
    echo -e "│       Failure → Evidence → Engineering Review               │"
    echo -e "│                                                             │"
    echo -e "│  CI/CD  •  Tests  •  Source  •  Traces  •  Runtime         │"
    echo -e "│                                                             │"
    echo -e "│                     by Plaicorp                             │"
    echo -e "│                    plaicorp.com                             │"
    echo -e "│                                                             │"
    echo -e "└─────────────────────────────────────────────────────────────┘"
}

if [[ ! -x "$EXECUTABLE" ]]; then
    echo -e "${RED}× Error: Executable not found at $EXECUTABLE${RESET}"
    exit 1
fi

REV=$(git rev-parse HEAD 2>/dev/null || echo "unknown")

print_header

if [[ "$MODE" == "live" ]]; then
    echo -e "${WHITE}DEMO${RESET}"
    echo -e "Finding and explaining a real software failure"
    echo ""
    echo -e "${CYAN}STEP 1 / 4 — STRESS TEST${RESET}"
    echo ""
    echo -e "Fuzz testing automatically generates unusual inputs to uncover"
    echo -e "software behavior developers may not anticipate."
    echo -e "We are feeding unexpected trace data into Riva's real trace"
    echo -e "ingestion boundary."
    echo ""
    echo -e "  Target         LoadNormalizedTraceFromJsonText"
    echo -e "  Instrumentation libFuzzer + ASan + UBSan"
    echo ""
    echo -e "${YELLOW}→ Starting fuzz campaign...${RESET}"
    echo ""

    # libFuzzer may add interesting mutations to its input corpus. Work from a
    # temporary copy so a local run cannot pollute the checked-in seed set.
    WORK_CORPUS=$(mktemp -d /tmp/riva-fuzz-corpus.XXXXXX)
    cp -a fuzz/corpus/json_trace_loader/. "$WORK_CORPUS/"

    # Run libFuzzer bounding to 10 seconds.
    # Output to terminal and tee to raw log.
    set +e
    "$EXECUTABLE" "$WORK_CORPUS" -artifact_prefix=fuzz/artifacts/json_trace_loader/ -max_total_time=10 2>&1 | tee "$LOG_DIR/fuzz.raw.log"
    FUZZ_EXIT=${PIPESTATUS[0]}
    set -e
    
    if [[ $FUZZ_EXIT -eq 0 ]]; then
        echo ""
        echo -e "${GREEN}✓ Fuzzing completed without finding a crash in the allotted time.${RESET}"
        echo -e "This demonstrates parser robustness. To view a crash demo, use --replay."
        exit 0
    else
        # Find the generated artifact
        ARTIFACT_PATH=$(find fuzz/artifacts/json_trace_loader -maxdepth 1 -type f \
            -name 'crash-*' -printf '%T@ %p\n' | sort -nr | awk 'NR == 1 { path = $2 } END { print path }')
        if [[ -z "$ARTIFACT_PATH" ]]; then
            echo -e "${RED}× Fuzzer exited non-zero but no crash artifact found.${RESET}"
            exit 1
        fi
        echo -e "\n${RED}× SOFTWARE FAILURE FOUND${RESET}"
        echo -e "The test discovered an input that causes the parser to fail."
    fi

elif [[ "$MODE" == "replay" ]]; then
    echo -e "${WHITE}DEMO${RESET}"
    echo -e "Can Riva turn a software failure into useful evidence?"
    echo ""
    echo -e "${CYAN}STEP 1 / 4 — DISCOVERED FAILURE${RESET}"
    echo ""
    echo -e "This input was previously discovered by LLVM libFuzzer while"
    echo -e "exercising Riva's production trace parser."
    echo ""
    
    if [[ ! -f "$ARTIFACT_PATH" ]]; then
        echo -e "${RED}× Error: Artifact not found at $ARTIFACT_PATH${RESET}"
        exit 1
    fi
fi

# Gather artifact stats
ART_SIZE=$(stat -c%s "$ARTIFACT_PATH" 2>/dev/null || stat -f%z "$ARTIFACT_PATH")
ART_SHA=$(sha256sum "$ARTIFACT_PATH" | awk '{print $1}')

if [[ "$MODE" == "replay" ]]; then
    echo -e "  Artifact  $(basename "$ARTIFACT_PATH")"
    echo -e "  Size      ${ART_SIZE} bytes"
    echo -e "  SHA-256   ${ART_SHA:0:16}..."
fi

echo ""
echo -e "${CYAN}STEP 2 / 4 — REPRODUCTION${RESET}"
echo ""
echo -e "We now run the exact same input again to verify that the failure"
echo -e "is repeatable rather than random."
echo ""
echo -e "${YELLOW}→ Executing production parser...${RESET}"
echo ""

set +e
$EXECUTABLE "$ARTIFACT_PATH" > "$LOG_DIR/reproduce.raw.log" 2> "$LOG_DIR/stderr.log"
REPRO_EXIT=$?
set -e

# A non-zero process result by itself is not proof of a memory-safety defect.
# Require an actual sanitizer diagnostic before creating an incident bundle.
SANITIZER=""
if grep -Eq "ERROR: AddressSanitizer|SUMMARY: AddressSanitizer" "$LOG_DIR/stderr.log"; then
    SANITIZER="AddressSanitizer"
elif grep -Eq "UndefinedBehaviorSanitizer|runtime error:" "$LOG_DIR/stderr.log"; then
    SANITIZER="UndefinedBehaviorSanitizer"
fi

if [[ $REPRO_EXIT -ne 0 && -n "$SANITIZER" ]]; then
    awk '/ERROR: AddressSanitizer|SUMMARY: AddressSanitizer|UndefinedBehaviorSanitizer|runtime error:/ { print; count += 1; if (count == 3) exit }' "$LOG_DIR/stderr.log"
    echo ""
    echo -e "${RED}× SOFTWARE FAILURE CONFIRMED${RESET}"
    echo -e "${GREEN}✓ same failure reproduced${RESET}"
    echo -e "${GREEN}✓ sanitizer diagnostic captured (${SANITIZER})${RESET}"
    echo ""
    echo -e "This shows the failure is repeatable."
else
    echo -e "${RED}× Replay did not reproduce a sanitizer-confirmed failure.${RESET}"
    echo -e "No incident bundle was created. Process exit: ${REPRO_EXIT}."
    exit 1
fi

echo ""
echo -e "${CYAN}STEP 3 / 4 — EVIDENCE${RESET}"
echo ""
echo -e "Riva preserves the failing input and the engineering context"
echo -e "needed to investigate what happened."
echo ""
echo -e "Riva collected:"
echo -e "  ${WHITE}[E1]${RESET} failing input"
echo -e "  ${WHITE}[E2]${RESET} sanitizer report"
echo -e "  ${WHITE}[E3]${RESET} source revision"
echo ""

INCIDENT_PATH=$(mktemp -d "$INCIDENT_DIR/RIVA-$(date +%Y)-XXXXXX")
INCIDENT_ID=$(basename "$INCIDENT_PATH")

cp "$ARTIFACT_PATH" "$INCIDENT_PATH/crash-input"
cp "$LOG_DIR/reproduce.raw.log" "$INCIDENT_PATH/"
cp "$LOG_DIR/stderr.log" "$INCIDENT_PATH/"

# Metadata
cat <<EOF > "$INCIDENT_PATH/metadata.json"
{
  "incident_id": "$INCIDENT_ID",
  "target": "LoadNormalizedTraceFromJsonText",
  "artifact_sha256": "$ART_SHA",
  "artifact_size": $ART_SIZE,
  "reproducible": true,
  "sanitizer": "$SANITIZER",
  "source_revision": "$REV",
  "timestamp": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
}
EOF

echo -e "${WHITE}SOURCE${RESET}"
echo -e "  revision    ${REV:0:7}..."
echo -e "  repository  Riva"
echo -e "  target      JSON Trace Loader"
echo -e "  sanitizer   ${SANITIZER}"
echo -e "  incident    ${INCIDENT_PATH}"
echo ""

echo -e "${CYAN}STEP 4 / 4 — ENGINEERING CONTEXT${RESET}"
echo ""
echo -e "The bundle preserves the parser target, source revision, exact"
echo -e "input hash, and observed sanitizer class. Riva's performance"
echo -e "analysis pipeline accepts valid trace documents; it does not"
echo -e "invent a performance diagnosis for a malformed crash artifact."
echo ""
echo -e "${GREEN}✓ incident evidence ready for engineering review${RESET}"

echo ""
print_footer
