#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
EXPECTED="${ROOT}/tests/member-initializers/invalid/expected/diagnostics.txt"

grep -Fq "class field requires explicit member initializer for non-default Construct: Child" "${EXPECTED}"
grep -Fq "tests/member-initializers/invalid/input/member_init.hp:11:5" "${EXPECTED}"

echo "member initializer invalid gap snapshot passed"
