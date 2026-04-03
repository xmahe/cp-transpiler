#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_private_methods"
OUT_DIR="${ROOT}/tests/.tmp/private_methods_out"
INPUT_DIR="${ROOT}/tests/private-methods/basic/input"
EXPECTED_DIR="${ROOT}/tests/private-methods/basic/expected"

mkdir -p "${ROOT}/tests/.tmp"
rm -rf "${OUT_DIR}"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"
"${BUILD_BIN}" -o "${OUT_DIR}" "${INPUT_DIR}"

diff -u "${EXPECTED_DIR}/private_demo.h" "${OUT_DIR}/private_demo.h"
diff -u "${EXPECTED_DIR}/private_demo.c" "${OUT_DIR}/private_demo.c"

echo "private method visibility test passed"
