#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
POSITIVE_HP="${ROOT}/tests/member-initializers/basic/input/member_init.hp"
POSITIVE_CP="${ROOT}/tests/member-initializers/basic/input/member_init.cp"
EXPECTED_H="${ROOT}/tests/member-initializers/basic/expected/member_init.h"
EXPECTED_C="${ROOT}/tests/member-initializers/basic/expected/member_init.c"
EXPECTED_DIAG="${ROOT}/tests/member-initializers/invalid/expected/diagnostics.txt"

grep -Fq "fn Parent::Construct(u32 primary_value, u32 secondary_value) -> void : primary(primary_value), secondary(secondary_value) {" "${POSITIVE_CP}"
grep -Fq "void Board___Parent___Construct(Board___Parent* self, u32 primary_value, u32 secondary_value);" "${EXPECTED_H}"
grep -Fq "Board___Child___Construct(&self->primary, primary_value);" "${EXPECTED_C}"
grep -Fq "Board___Child___Destroy(&self->secondary);" "${EXPECTED_C}"
grep -Fq "class field requires explicit member initializer for non-default Construct: Child" "${EXPECTED_DIAG}"

echo "member initializer gap snapshot passed"
