#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_example"
TMP_DIR="${ROOT}/tests/.tmp"
OUT_DIR="${TMP_DIR}/out_example"

mkdir -p "${TMP_DIR}"
rm -rf "${OUT_DIR}"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"
"${BUILD_BIN}" -o "${OUT_DIR}" "${ROOT}/tests/example-transpile/basic/input"

test -f "${OUT_DIR}/example.h"
test -f "${OUT_DIR}/example.c"

grep -q "typedef struct Board___DeviceController" "${OUT_DIR}/example.h"
grep -q "Board___DeviceController___Transfer" "${OUT_DIR}/example.h"
grep -q "Board___DeviceController___Read" "${OUT_DIR}/example.h"
grep -q "Board___DeviceRepository___construct" "${OUT_DIR}/example.h"
grep -q "__cplus_maybe_u32" "${OUT_DIR}/example.h"
grep -q "Board___DeviceController___instance_count" "${OUT_DIR}/example.c"
grep -q "Board___Logger___instance_count" "${OUT_DIR}/example.c"
grep -q "Board___ComputeChecksum" "${OUT_DIR}/example.c"

echo "example transpilation test passed"
