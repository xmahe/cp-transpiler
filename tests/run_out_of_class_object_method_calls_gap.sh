#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_out_of_class_object_method_calls"
TMP_DIR="${ROOT}/tests/.tmp"
INPUT_DIR="${ROOT}/tests/out-of-class-object-method-calls/basic/input"

mkdir -p "${TMP_DIR}"
rm -rf "${TMP_DIR}/out_out_of_class_object_method_calls"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"
"${BUILD_BIN}" -o "${TMP_DIR}/out_out_of_class_object_method_calls" "${INPUT_DIR}"
diff -u "${ROOT}/tests/out-of-class-object-method-calls/basic/expected/thermo.h" "${TMP_DIR}/out_out_of_class_object_method_calls/thermo.h"
diff -u "${ROOT}/tests/out-of-class-object-method-calls/basic/expected/thermo.c" "${TMP_DIR}/out_out_of_class_object_method_calls/thermo.c"

echo "out-of-class object method call rewriting test passed"
