#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_raii_return_value"
TMP_DIR="${ROOT}/tests/.tmp"
OUT_DIR="${TMP_DIR}/out_raii_return_value"

mkdir -p "${TMP_DIR}"
rm -rf "${OUT_DIR}"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"
"${BUILD_BIN}" -o "${OUT_DIR}" "${ROOT}/tests/raii-return-value/basic/input"
diff -u "${ROOT}/tests/raii-return-value/basic/expected/raii.h" "${OUT_DIR}/raii.h"
diff -u "${ROOT}/tests/raii-return-value/basic/expected/raii.c" "${OUT_DIR}/raii.c"

echo "raii return value test passed"
