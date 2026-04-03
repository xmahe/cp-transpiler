#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_member_initializer_overloads"
TMP_DIR="${ROOT}/tests/.tmp"
OUT_DIR="${TMP_DIR}/out_member_initializer_overloads"

mkdir -p "${TMP_DIR}"
rm -rf "${OUT_DIR}"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"
"${BUILD_BIN}" -o "${OUT_DIR}" "${ROOT}/tests/member-initializer-overloads/basic/input"
diff -u "${ROOT}/tests/member-initializer-overloads/basic/expected/overload.h" "${OUT_DIR}/overload.h"
diff -u "${ROOT}/tests/member-initializer-overloads/basic/expected/overload.c" "${OUT_DIR}/overload.c"

set +e
"${BUILD_BIN}" -o "${TMP_DIR}/out_member_initializer_overloads_invalid" "${ROOT}/tests/member-initializer-overloads/invalid/input" >"${TMP_DIR}/member_initializer_overloads_invalid.log" 2>&1
status=$?
set -e
if [ "${status}" -eq 0 ]; then
    echo "expected invalid member initializer overload test to fail"
    exit 1
fi
diff -u "${ROOT}/tests/member-initializer-overloads/invalid/expected/diagnostics.txt" "${TMP_DIR}/member_initializer_overloads_invalid.log"

echo "member initializer overloads test passed"
