#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_raii_nested_return"
TMP_DIR="${ROOT}/tests/.tmp"
OUT_DIR="${TMP_DIR}/out_raii_nested_return"

mkdir -p "${TMP_DIR}"
rm -rf "${OUT_DIR}"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"
"${BUILD_BIN}" -o "${OUT_DIR}" "${ROOT}/tests/raii-nested-return/basic/input"
diff -u "${ROOT}/tests/raii-nested-return/basic/expected/raii.h" "${OUT_DIR}/raii.h"
diff -u "${ROOT}/tests/raii-nested-return/basic/expected/raii.c" "${OUT_DIR}/raii.c"

echo "raii nested return test passed"
