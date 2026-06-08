#!/usr/bin/env bash
set -euo pipefail

missing=0

required_files=(
  "README.md"
  "CMakeLists.txt"
  "source/PluginProcessor.h"
  "source/PluginProcessor.cpp"
  ".github/workflows/build-vst.yml"
  "docs/case-studies/issue-1/README.md"
  "docs/case-studies/issue-1/research.md"
  "docs/case-studies/issue-1/architecture.md"
  "docs/case-studies/issue-1/action-plan.md"
)

for path in "${required_files[@]}"; do
  if [[ ! -f "$path" ]]; then
    echo "Missing required file: $path" >&2
    missing=1
  fi
done

if [[ "$missing" -ne 0 ]]; then
  exit 1
fi

grep -q "docs/case-studies/issue-1" README.md
grep -q "JUCE" docs/case-studies/issue-1/research.md
grep -q "Dear Reality" docs/case-studies/issue-1/research.md
grep -q "FL Studio" docs/case-studies/issue-1/research.md
grep -q "VST3" docs/case-studies/issue-1/architecture.md
grep -q "Win32" .github/workflows/build-vst.yml
grep -q "x64" .github/workflows/build-vst.yml
grep -q "actions/upload-artifact" .github/workflows/build-vst.yml

echo "Preparation files and workflow markers are present."
