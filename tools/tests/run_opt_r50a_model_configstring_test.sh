#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
mkdir -p "$tmp/src/qcommon" "$tmp/src/server"
cp src/qcommon/cod1_model_configstring_r50.h "$tmp/src/qcommon/"
cp src/server/sv_cod1_model_configstring_r50.h "$tmp/src/server/"
sed -n '/^\/\* One callback-free server scan replaces a lease check around every slot/,/^static int WeaponRegistrationModel(/{ /^static int WeaponRegistrationModel(/d; p; }' src/game/gsc_host/g_cod1_weapon_registration.inc > "$tmp/lookup.inc"
test "$(git hash-object "$tmp/src/qcommon/cod1_model_configstring_r50.h")" = 302e7a3f469911b7aefc504ff7bd9fa86dccb6d2
test "$(git hash-object "$tmp/src/server/sv_cod1_model_configstring_r50.h")" = 507139aa1c0b92c3ce4a8ecf6c7586fa8e7011dd
test "$(git hash-object "$tmp/lookup.inc")" = 2208f03811fa12e9522d98b078eeef12d5e520ca
cp tools/tests/opt_r50a_model_configstring_test.c "$tmp/test.c"
cc -std=c11 -O2 -Wall -Wextra -Werror "$tmp/test.c" -o "$tmp/test"
"$tmp/test"
