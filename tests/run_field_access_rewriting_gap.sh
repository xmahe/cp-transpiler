#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
EXPECTED="${ROOT}/tests/out-of-class-field-access/basic/expected/thermo.c"

grep -q "self->reading = initial_reading;" "${EXPECTED}"
grep -q "self->reading += offset;" "${EXPECTED}"
grep -q "return self->reading;" "${EXPECTED}"

echo "field access rewriting snapshot test passed"
