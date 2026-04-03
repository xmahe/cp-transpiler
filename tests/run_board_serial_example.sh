#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BUILD_BIN="${ROOT}/tests/.tmp/cplus_board_serial"
OUT_DIR="${ROOT}/tests/.tmp/out_board_serial"
INPUT_DIR="${ROOT}/tests/examples/board-serial/basic/input"

mkdir -p "${ROOT}/tests/.tmp"
rm -rf "${OUT_DIR}"

clang++ -std=c++20 -Isrc $(find "${ROOT}/src" -name '*.cpp' | sort) -o "${BUILD_BIN}"
"${BUILD_BIN}" -o "${OUT_DIR}" "${INPUT_DIR}"

test -f "${OUT_DIR}/board_serial.h"
test -f "${OUT_DIR}/board_serial.c"

grep -q "typedef struct Board___SerialPort" "${OUT_DIR}/board_serial.h"
grep -q "__cplus_maybe_u32" "${OUT_DIR}/board_serial.h"
grep -q "Board___BootSerial" "${OUT_DIR}/board_serial.c"
grep -q "return baud_rate;" "${OUT_DIR}/board_serial.c"
grep -q "SerialSnapshot snapshot;" "${OUT_DIR}/board_serial.c"
grep -q "SerialStats stats;" "${OUT_DIR}/board_serial.c"

echo "board serial example transpilation test passed"
