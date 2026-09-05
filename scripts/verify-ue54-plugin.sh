#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
plugin_path="$repo_root/ue/Plugins/RivaEditor/RivaEditor.uplugin"
package_dir="${1:-/tmp/RivaEditor-UE54-$$}"

if [[ -z "${UE_ROOT:-}" ]]; then
  printf 'error: UE_ROOT must point to an Unreal Engine 5.4 installation.\n' >&2
  exit 2
fi

run_uat="$UE_ROOT/Engine/Build/BatchFiles/RunUAT.sh"
build_version="$UE_ROOT/Engine/Build/Build.version"

if [[ ! -x "$run_uat" ]]; then
  printf 'error: RunUAT.sh is not executable at %s\n' "$run_uat" >&2
  exit 2
fi

if [[ ! -f "$build_version" ]]; then
  printf 'error: Unreal Build.version was not found at %s\n' "$build_version" >&2
  exit 2
fi

if ! grep -Eq '"MajorVersion"[[:space:]]*:[[:space:]]*5' "$build_version" ||
   ! grep -Eq '"MinorVersion"[[:space:]]*:[[:space:]]*4' "$build_version"; then
  printf 'error: UE_ROOT is not an Unreal Engine 5.4 installation.\n' >&2
  exit 2
fi

if [[ ! -f "$plugin_path" ]]; then
  printf 'error: plugin descriptor was not found at %s\n' "$plugin_path" >&2
  exit 2
fi

if [[ -d "$package_dir" ]]; then
  if [[ -n "$(find "$package_dir" -mindepth 1 -maxdepth 1 -print -quit)" ]]; then
    printf 'error: package output directory is not empty: %s\n' "$package_dir" >&2
    exit 2
  fi
fi

printf 'Packaging RivaEditor for Unreal Engine 5.4 (Linux)...\n'
printf 'Output: %s\n' "$package_dir"

"$run_uat" BuildPlugin \
  -Plugin="$plugin_path" \
  -Package="$package_dir" \
  -TargetPlatforms=Linux

printf 'RivaEditor UE 5.4 package completed: %s\n' "$package_dir"
