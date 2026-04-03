#!/usr/bin/env bash
set -euo pipefail

if [ "$#" -ne 5 ]; then
    echo "usage: ctest_fixture.sh <compiler> <pass|fail> <input_dir> <expected_dir> <work_dir>"
    exit 2
fi

COMPILER="$1"
MODE="$2"
INPUT_DIR="$3"
EXPECTED_DIR="$4"
WORK_DIR="$5"
OUT_DIR="${WORK_DIR}/out"
LOG_FILE="${WORK_DIR}/diagnostics.log"

rm -rf "${WORK_DIR}"
mkdir -p "${OUT_DIR}"

case "${MODE}" in
    pass)
        "${COMPILER}" -o "${OUT_DIR}" "${INPUT_DIR}"

        while IFS= read -r expected_file; do
            relative_path="${expected_file#${EXPECTED_DIR}/}"
            actual_file="${OUT_DIR}/${relative_path}"
            if [ ! -f "${actual_file}" ]; then
                echo "missing generated file: ${relative_path}"
                exit 1
            fi
            diff -u -B "${expected_file}" "${actual_file}"
        done < <(find "${EXPECTED_DIR}" -type f | sort)
        ;;
    fail)
        set +e
        "${COMPILER}" -o "${OUT_DIR}" "${INPUT_DIR}" >"${LOG_FILE}" 2>&1
        status=$?
        set -e

        if [ "${status}" -eq 0 ]; then
            echo "expected transpilation to fail"
            exit 1
        fi

        diff -u "${EXPECTED_DIR}/diagnostics.txt" "${LOG_FILE}"
        ;;
    *)
        echo "unknown mode: ${MODE}"
        exit 2
        ;;
esac
