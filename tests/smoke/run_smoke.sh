#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_smoke"
OUT_DIR="${ROOT}/tests/.tmp/out"
INPUT_DIR="${ROOT}/tests/smoke/basic/input"
EXPECTED_DIR="${ROOT}/tests/smoke/basic/expected"

mkdir -p "${ROOT}/tests/.tmp"
rm -rf "${OUT_DIR}"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"
"${BUILD_BIN}" -o "${OUT_DIR}" "${INPUT_DIR}"

diff -u "${EXPECTED_DIR}/demo.h" "${OUT_DIR}/demo.h"
diff -u "${EXPECTED_DIR}/demo.c" "${OUT_DIR}/demo.c"

echo "smoke test passed"
