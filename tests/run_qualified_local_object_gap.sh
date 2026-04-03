#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_qualified_local_object"
TMP_DIR="${ROOT}/tests/.tmp"
INPUT_DIR="${ROOT}/tests/qualified-local-object/basic/input"

mkdir -p "${TMP_DIR}"
rm -rf "${TMP_DIR}/out_qualified_local_object"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"
"${BUILD_BIN}" -o "${TMP_DIR}/out_qualified_local_object" "${INPUT_DIR}"
diff -u "${ROOT}/tests/qualified-local-object/basic/expected/qualified_local_object.h" "${TMP_DIR}/out_qualified_local_object/qualified_local_object.h"
diff -u "${ROOT}/tests/qualified-local-object/basic/expected/qualified_local_object.c" "${TMP_DIR}/out_qualified_local_object/qualified_local_object.c"

echo "qualified local object lowering regression test passed"
