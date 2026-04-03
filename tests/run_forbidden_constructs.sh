#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_forbidden_tests"
TMP_DIR="${ROOT}/tests/.tmp"

mkdir -p "${TMP_DIR}"
rm -rf "${TMP_DIR}/out_forbidden"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"

set +e
"${BUILD_BIN}" -o "${TMP_DIR}/out_forbidden" "${ROOT}/tests/forbidden-constructs/invalid/input" >"${TMP_DIR}/forbidden.log" 2>&1
status=$?
set -e
if [ "${status}" -eq 0 ]; then
    echo "expected forbidden constructs test to fail"
    exit 1
fi

diff -u "${ROOT}/tests/forbidden-constructs/invalid/expected/diagnostics.txt" "${TMP_DIR}/forbidden.log"

echo "forbidden constructs test passed"
