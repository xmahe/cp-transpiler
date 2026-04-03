#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_gap_tests"
TMP_DIR="${ROOT}/tests/.tmp"

mkdir -p "${TMP_DIR}"
rm -rf "${TMP_DIR}/out_static" "${TMP_DIR}/out_ns"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"

failed=0

"${BUILD_BIN}" -o "${TMP_DIR}/out_static" "${ROOT}/tests/static-vars/basic/input"
if ! diff -u "${ROOT}/tests/static-vars/basic/expected/static_vars.h" "${TMP_DIR}/out_static/static_vars.h"; then
    echo "static-vars header mismatch"
    failed=1
fi
if ! diff -u "${ROOT}/tests/static-vars/basic/expected/static_vars.c" "${TMP_DIR}/out_static/static_vars.c"; then
    echo "static-vars source mismatch"
    failed=1
fi

set +e
"${BUILD_BIN}" -o "${TMP_DIR}/out_ns" "${ROOT}/tests/namespace-top-level/invalid/input" >"${TMP_DIR}/namespace_top_level.log" 2>&1
status=$?
set -e
if [ "${status}" -eq 0 ]; then
    echo "expected top-level namespace violation to fail"
    failed=1
else
    if ! diff -u "${ROOT}/tests/namespace-top-level/invalid/expected/diagnostics.txt" "${TMP_DIR}/namespace_top_level.log"; then
        echo "namespace-top-level diagnostic mismatch"
        failed=1
    fi
fi

if [ "${failed}" -ne 0 ]; then
    exit 1
fi

bash "${ROOT}/tests/run_field_access_rewriting_gap.sh"
bash "${ROOT}/tests/run_out_of_class_method_calls_gap.sh"
bash "${ROOT}/tests/run_member_initializers_gap.sh"
bash "${ROOT}/tests/run_member_initializers_invalid_gap.sh"
echo "gap tests passed"
