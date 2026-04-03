#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_member_initializers"
TMP_DIR="${ROOT}/tests/.tmp"
OUT_DIR="${TMP_DIR}/out_member_initializers"

mkdir -p "${TMP_DIR}"
rm -rf "${OUT_DIR}"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"
"${BUILD_BIN}" -o "${OUT_DIR}" "${ROOT}/tests/member-initializers/basic/input"
diff -u "${ROOT}/tests/member-initializers/basic/expected/member_init.h" "${OUT_DIR}/member_init.h"
diff -u "${ROOT}/tests/member-initializers/basic/expected/member_init.c" "${OUT_DIR}/member_init.c"

set +e
"${BUILD_BIN}" -o "${TMP_DIR}/out_member_initializers_invalid" "${ROOT}/tests/member-initializers/invalid/input" >"${TMP_DIR}/member_initializers_invalid.log" 2>&1
status=$?
set -e
if [ "${status}" -eq 0 ]; then
    echo "expected invalid member initializer test to fail"
    exit 1
fi
diff -u "${ROOT}/tests/member-initializers/invalid/expected/diagnostics.txt" "${TMP_DIR}/member_initializers_invalid.log"

echo "member initializers test passed"
