#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_member_subobjects"
TMP_DIR="${ROOT}/tests/.tmp"
OUT_DIR="${TMP_DIR}/out_member_subobjects"

mkdir -p "${TMP_DIR}"
rm -rf "${OUT_DIR}"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"
"${BUILD_BIN}" -o "${OUT_DIR}" "${ROOT}/tests/member-subobjects/basic/input"
diff -u "${ROOT}/tests/member-subobjects/basic/expected/member.h" "${OUT_DIR}/member.h"
diff -u "${ROOT}/tests/member-subobjects/basic/expected/member.c" "${OUT_DIR}/member.c"

set +e
"${BUILD_BIN}" -o "${TMP_DIR}/out_member_subobjects_invalid" "${ROOT}/tests/member-subobjects/invalid/input" >"${TMP_DIR}/member_subobjects_invalid.log" 2>&1
status=$?
set -e
if [ "${status}" -eq 0 ]; then
    echo "expected invalid member subobject test to fail"
    exit 1
fi
diff -u "${ROOT}/tests/member-subobjects/invalid/expected/diagnostics.txt" "${TMP_DIR}/member_subobjects_invalid.log"

echo "member subobjects test passed"
