#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_body_pair_tests"
TMP_DIR="${ROOT}/tests/.tmp"

mkdir -p "${TMP_DIR}"
rm -rf "${TMP_DIR}/out_body_paired" "${TMP_DIR}/out_bad_body_paired"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"

"${BUILD_BIN}" -o "${TMP_DIR}/out_body_paired" "${ROOT}/tests/body-parsing/basic/input"
diff -u "${ROOT}/tests/body-parsing/basic/expected/body.h" "${TMP_DIR}/out_body_paired/body.h"
diff -u "${ROOT}/tests/body-parsing/basic/expected/body.c" "${TMP_DIR}/out_body_paired/body.c"

set +e
"${BUILD_BIN}" -o "${TMP_DIR}/out_bad_body_paired" "${ROOT}/tests/body-parsing/invalid/input" >"${TMP_DIR}/bad_body_paired.log" 2>&1
status=$?
set -e
if [ "${status}" -eq 0 ]; then
    echo "expected invalid paired body test to fail"
    exit 1
fi
diff -u "${ROOT}/tests/body-parsing/invalid/expected/diagnostics.txt" "${TMP_DIR}/bad_body_paired.log"

echo "paired body parsing test passed"
