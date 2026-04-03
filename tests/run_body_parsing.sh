#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_body_tests"
TMP_DIR="${ROOT}/tests/.tmp"

mkdir -p "${TMP_DIR}"
rm -rf "${TMP_DIR}/out_body" "${TMP_DIR}/out_bad_body"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"

"${BUILD_BIN}" -o "${TMP_DIR}/out_body" "${ROOT}/tests/body-parsing/basic/input/body.cp"
diff -u "${ROOT}/tests/body-parsing/basic/expected/body.h" "${TMP_DIR}/out_body/body.h"
diff -u "${ROOT}/tests/body-parsing/basic/expected/body.c" "${TMP_DIR}/out_body/body.c"

set +e
"${BUILD_BIN}" -o "${TMP_DIR}/out_bad_body" "${ROOT}/tests/body-parsing/invalid/input/body_bad.cp" >"${TMP_DIR}/bad_body.log" 2>&1
status=$?
set -e
if [ "${status}" -eq 0 ]; then
    echo "expected invalid body test to fail"
    exit 1
fi
diff -u "${ROOT}/tests/body-parsing/invalid/expected/diagnostics.txt" "${TMP_DIR}/bad_body.log"

echo "body parsing test passed"
