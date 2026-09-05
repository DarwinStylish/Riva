#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
timestamp="$(date -u +%Y%m%dT%H%M%SZ)"
build_dir="$repo_root/build/evidence-release"
output_dir="$repo_root/artifacts/verification/$timestamp"
allow_dirty=false

usage() {
  printf 'Usage: %s [--build-dir PATH] [--output-dir PATH] [--allow-dirty]\n' "$0"
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-dir)
      [[ $# -ge 2 ]] || { usage >&2; exit 2; }
      build_dir="$2"
      shift 2
      ;;
    --output-dir)
      [[ $# -ge 2 ]] || { usage >&2; exit 2; }
      output_dir="$2"
      shift 2
      ;;
    --allow-dirty)
      allow_dirty=true
      shift
      ;;
    --help|-h)
      usage
      exit 0
      ;;
    *)
      printf 'error: unknown option: %s\n' "$1" >&2
      usage >&2
      exit 2
      ;;
  esac
done

revision="$(git -C "$repo_root" rev-parse HEAD 2>/dev/null || printf 'unknown')"
source_state="clean"
if [[ -n "$(git -C "$repo_root" status --porcelain 2>/dev/null)" ]]; then
  source_state="dirty"
  if [[ "$allow_dirty" != true ]]; then
    printf 'error: source tree is dirty; commit or stash changes before collecting final evidence\n' >&2
    printf '       use --allow-dirty only for local validation\n' >&2
    exit 2
  fi
fi

if [[ -d "$output_dir" ]]; then
  if [[ -n "$(find "$output_dir" -mindepth 1 -maxdepth 1 -print -quit)" ]]; then
    printf 'error: evidence output directory is not empty: %s\n' "$output_dir" >&2
    exit 2
  fi
fi
mkdir -p "$output_dir/logs" "$output_dir/reports"

run_logged() {
  local log_path="$1"
  shift
  printf 'Command:' > "$log_path"
  printf ' %q' "$@" >> "$log_path"
  printf '\n\n' >> "$log_path"
  "$@" >> "$log_path" 2>&1
}

run_logged "$output_dir/logs/configure.log" \
  cmake -S "$repo_root" -B "$build_dir" \
    -DCMAKE_BUILD_TYPE=Release \
    -DRIVA_BUILD_TESTS=ON \
    -DRIVA_BUILD_CLI=ON \
    -DRIVA_WARNINGS_AS_ERRORS=ON

run_logged "$output_dir/logs/build.log" \
  cmake --build "$build_dir" --config Release --parallel 4

run_logged "$output_dir/logs/ctest.log" \
  ctest --test-dir "$build_dir" -C Release --output-on-failure --no-tests=error

run_logged "$output_dir/logs/install.log" \
  cmake --install "$build_dir" --config Release --prefix "$output_dir/install"

riva_cli="$build_dir/riva"
if [[ ! -x "$riva_cli" && -x "$build_dir/Release/riva.exe" ]]; then
  riva_cli="$build_dir/Release/riva.exe"
fi
if [[ ! -x "$riva_cli" ]]; then
  printf 'error: built Riva CLI was not found in %s\n' "$build_dir" >&2
  exit 1
fi

sample_count=0
for sample in "$repo_root"/samples/spike_*.json; do
  sample_name="$(basename "$sample" .json)"
  "$riva_cli" analyze "$sample" --format json \
    --output "$output_dir/reports/$sample_name.json"
  "$riva_cli" analyze "$sample" --format markdown \
    --output "$output_dir/reports/$sample_name.md"
  sample_count=$((sample_count + 1))
done

set +e
"$riva_cli" compare \
  "$repo_root/samples/spike_cpu_game_thread.json" \
  "$repo_root/samples/spike_shader_compile.json" \
  --output "$output_dir/reports/controlled-regression.md" \
  > "$output_dir/logs/compare.stdout.log" \
  2> "$output_dir/logs/compare.stderr.log"
compare_exit=$?
set -e
if [[ $compare_exit -ne 3 ]]; then
  printf 'error: controlled regression returned %d, expected 3\n' "$compare_exit" >&2
  exit 1
fi

set +e
"$riva_cli" check-budget \
  --budget "$repo_root/samples/sample_budget.json" \
  --trace "$repo_root/samples/spike_shader_compile.json" \
  > "$output_dir/logs/budget.stdout.log" \
  2> "$output_dir/logs/budget.stderr.log"
budget_exit=$?
set -e
if [[ $budget_exit -ne 3 ]]; then
  printf 'error: controlled budget breach returned %d, expected 3\n' "$budget_exit" >&2
  exit 1
fi

{
  printf 'Source revision: %s\n' "$revision"
  printf 'Source state: %s\n' "$source_state"
  printf 'Host: %s\n' "$(uname -srm)"
  printf 'CMake:\n'
  cmake --version
  printf 'Compiler:\n'
  "${CXX:-c++}" --version
} > "$output_dir/logs/environment.log"

ue54_status="not_run"
if [[ -n "${UE_ROOT:-}" ]]; then
  if "$repo_root/scripts/verify-ue54-plugin.sh" "$output_dir/ue54-package" \
      > "$output_dir/logs/ue54-build.log" 2>&1; then
    ue54_status="passed"
  else
    ue54_status="failed"
    printf 'error: UE 5.4 BuildPlugin verification failed; see %s\n' \
      "$output_dir/logs/ue54-build.log" >&2
    exit 1
  fi
else
  printf 'UE_ROOT was not set; Unreal Engine 5.4 BuildPlugin verification was not run.\n' \
    > "$output_dir/logs/ue54-build.log"
fi

cat > "$output_dir/status.json" <<EOF
{
  "schema_version": 1,
  "generated_at_utc": "$timestamp",
  "source_revision": "$revision",
  "source_state": "$source_state",
  "standalone_release_build": "passed",
  "standalone_install": "passed",
  "ctest": "passed",
  "sample_reports_generated": $sample_count,
  "comparison_exit_code": $compare_exit,
  "budget_exit_code": $budget_exit,
  "ue54_buildplugin": "$ue54_status"
}
EOF

cat > "$output_dir/SUMMARY.md" <<EOF
# Riva Verification Evidence

- Generated: $timestamp
- Source revision: $revision
- Source state: $source_state
- Standalone strict Release build: passed
- Standalone install tree: passed
- CTest suite: passed
- Worked sample reports: $sample_count JSON and $sample_count Markdown
- Controlled comparison gate: passed (expected exit code 3)
- Controlled budget gate: passed (expected exit code 3)
- Unreal Engine 5.4 BuildPlugin: $ue54_status

The Unreal status is intentionally reported as \`not_run\` when \`UE_ROOT\` is
not available. This evidence collector never infers or fabricates an Unreal
build result.
EOF

checksum_file="$output_dir/SHA256SUMS"
: > "$checksum_file"
while IFS= read -r evidence_file; do
  relative_path="${evidence_file#"$output_dir/"}"
  if command -v sha256sum >/dev/null 2>&1; then
    checksum="$(sha256sum "$evidence_file" | awk '{print $1}')"
  else
    checksum="$(shasum -a 256 "$evidence_file" | awk '{print $1}')"
  fi
  printf '%s  %s\n' "$checksum" "$relative_path" >> "$checksum_file"
done < <(find "$output_dir" -type f ! -name SHA256SUMS | LC_ALL=C sort)

printf 'Verification evidence written to %s\n' "$output_dir"
