#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
base=c35553c32221885b2117123cf6ac90b9bef9a6dc
p=src/game/cod1_gsc
git show "$base:$p/gsc_vm_results.inc" | awk '/^static int results_operator\(/ {active=1} /^\/\* 1: result delivered/ {active=0} active {print}' > "$tmp/operator.inc"
git show "$base:$p/gsc_vm_scalar.inc" | awk '/^static int32_t scalar_signed\(/ {active=1} /^static int scalar_binary\(/ {active=0} active {print}' > "$tmp/scalar.inc"
git show "$base:$p/gsc_vm_integer_bitwise.inc" | awk '/^static int integer_bitwise_unary\(/ {active=1} /^static int integer_bitwise_mutation\(/ {active=0} active {print}' > "$tmp/bitwise.inc"
test -s "$tmp/operator.inc" && test -s "$tmp/scalar.inc" && test -s "$tmp/bitwise.inc"
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror -Wno-unused-function -Wno-misleading-indentation -frounding-math}"
"${CC:-cc}" "${flags[@]}" -I"$p" -I"$tmp" tools/tests/opt_r56b1_binary_test.c -lm -o "$tmp/test"
"$tmp/test"
