#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_tests"
TMP_DIR="${ROOT}/tests/.tmp"

mkdir -p "${TMP_DIR}"
rm -rf "${TMP_DIR}/out_interface" "${TMP_DIR}/out_maybe"
rm -rf "${TMP_DIR}/out_namespace"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"

"${ROOT}/tests/smoke/run_smoke.sh"

"${BUILD_BIN}" -o "${TMP_DIR}/out_interface" "${ROOT}/tests/interface-fulfillment/basic/input"
diff -u "${ROOT}/tests/interface-fulfillment/basic/expected/interface.h" "${TMP_DIR}/out_interface/interface.h"
diff -u "${ROOT}/tests/interface-fulfillment/basic/expected/interface.c" "${TMP_DIR}/out_interface/interface.c"

"${BUILD_BIN}" -o "${TMP_DIR}/out_maybe" "${ROOT}/tests/maybe-safety/basic/input"
diff -u "${ROOT}/tests/maybe-safety/basic/expected/maybe.h" "${TMP_DIR}/out_maybe/maybe.h"
diff -u "${ROOT}/tests/maybe-safety/basic/expected/maybe.c" "${TMP_DIR}/out_maybe/maybe.c"

set +e
"${BUILD_BIN}" -o "${TMP_DIR}/out_bad_maybe" "${ROOT}/tests/maybe-safety/invalid/input" >"${TMP_DIR}/bad_maybe.log" 2>&1
status=$?
set -e
if [ "${status}" -eq 0 ]; then
    echo "expected invalid maybe test to fail"
    exit 1
fi
diff -u "${ROOT}/tests/maybe-safety/invalid/expected/diagnostics.txt" "${TMP_DIR}/bad_maybe.log"

set +e
"${BUILD_BIN}" -o "${TMP_DIR}/out_namespace" "${ROOT}/tests/namespace-top-level/invalid/input" >"${TMP_DIR}/namespace_top_level.log" 2>&1
status=$?
set -e
if [ "${status}" -eq 0 ]; then
    echo "expected top-level namespace violation to fail"
    exit 1
fi
diff -u "${ROOT}/tests/namespace-top-level/invalid/expected/diagnostics.txt" "${TMP_DIR}/namespace_top_level.log"

"${ROOT}/tests/run_forbidden_constructs.sh"
bash "${ROOT}/tests/run_body_parsing_paired.sh"
bash "${ROOT}/tests/run_example_transpile.sh"
bash "${ROOT}/tests/run_board_serial_example.sh"
bash "${ROOT}/tests/run_out_of_class_methods.sh"
bash "${ROOT}/tests/run_out_of_class_method_calls_gap.sh"
bash "${ROOT}/tests/run_out_of_class_object_method_calls_gap.sh"
bash "${ROOT}/tests/run_raii_return.sh"
bash "${ROOT}/tests/run_raii_return_value.sh"
bash "${ROOT}/tests/run_raii_fallthrough.sh"
bash "${ROOT}/tests/run_raii_nested_return.sh"
bash "${ROOT}/tests/run_raii_nested_return_value.sh"

echo "feature tests passed"
