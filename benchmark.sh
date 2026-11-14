#!/usr/bin/env bash
# Builds the benchmarks with optimizations and runs them.
#
# Usage: ./benchmark.sh [module] [Catch2 arguments...]
#   module  core or particles (default: all modules that have benchmarks)
# Further arguments go to the benchmark program, for example a part of a test name:
#   ./benchmark.sh particles "simulating a full pool*"
set -euo pipefail

cd "$(dirname "$0")"

preset="release"
modules=(core particles)
if [[ $# -gt 0 ]]; then
    modules=("$1")
    shift
fi

targets=()
for module in "${modules[@]}"; do
    targets+=("ember_${module}_benchmarks")
done

cmake --preset "${preset}"
cmake --build --preset "${preset}" --target "${targets[@]}"

for target in "${targets[@]}"; do
    program="$(find "build/${preset}" -type f -name "${target}" -print -quit)"
    echo
    echo "=== ${target}"
    "${program}" "$@"
done
