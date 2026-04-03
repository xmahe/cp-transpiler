#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_inject_bind"
TMP_DIR="${ROOT}/tests/.tmp"
OUT_DIR="${TMP_DIR}/out_inject_bind"

mkdir -p "${TMP_DIR}"
rm -rf "${OUT_DIR}"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"
"${BUILD_BIN}" -o "${OUT_DIR}" "${ROOT}/tests/inject-bind/basic/input"
diff -u "${ROOT}/tests/inject-bind/basic/expected/inject.h" "${OUT_DIR}/inject.h"
diff -u "${ROOT}/tests/inject-bind/basic/expected/inject.c" "${OUT_DIR}/inject.c"

set +e
"${BUILD_BIN}" -o "${TMP_DIR}/out_inject_bind_invalid" "${ROOT}/tests/inject-bind/invalid/input" >"${TMP_DIR}/inject_bind_invalid.log" 2>&1
status=$?
set -e
if [ "${status}" -eq 0 ]; then
    echo "expected invalid inject bind test to fail"
    exit 1
fi
diff -u "${ROOT}/tests/inject-bind/invalid/expected/diagnostics.txt" "${TMP_DIR}/inject_bind_invalid.log"

echo "inject bind test passed"
