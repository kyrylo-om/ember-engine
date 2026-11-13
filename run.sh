#!/usr/bin/env bash
# Builds one example and runs it, without building or running the tests.
#
# Usage: ./run.sh [example] [preset]
#   example  name of a folder in examples/ (default: yozakura)
#   preset   debug or release (default: release, which runs faster)
set -euo pipefail

example="${1:-yozakura}"
preset="${2:-release}"

cd "$(dirname "$0")"

if [[ ! -d "examples/${example}" ]]; then
    echo "There is no example '${example}'. Available examples:" >&2
    find examples -mindepth 1 -maxdepth 1 -type d -printf '  %f\n' >&2
    exit 1
fi

cmake --preset "${preset}"
cmake --build --preset "${preset}" --target "${example}"
exec "build/${preset}/examples/${example}/${example}"
