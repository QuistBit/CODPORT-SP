#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
base=eddb596379db41403a08f5315c978ec718f6bdc3
session=src/game/cod1_gsc/gsc_session.c
item=src/game/gsc_host/g_cod1_item_precache_host.inc
reg=src/game/gsc_host/g_cod1_weapon_registration.inc
git show "$base:$session" | sed -n '/^int gsc_session_snapshot(/,/^}/p' > "$tmp/snapshot.inc"
sed -n '/^\/\* Item registration needs only these three view fields/,/^}/p' "$session" > "$tmp/clock.inc"
git show "$base:$item" | sed -n '/^static int GscTestItemCurrentChecked(/,/^static int GscTestItemAdmission(/{ /^static int GscTestItemAdmission(/d; p; }' > "$tmp/item.old"
sed -n '/^static int GscTestItemCurrentClockChecked(/,/^static int GscTestItemAdmission(/{ /^static int GscTestItemAdmission(/d; p; }' "$item" > "$tmp/item.new"
git show "$base:$reg" | sed -n '/^static int WeaponRegistrationScope(/,/^static int WeaponRegistrationGet(/{ /^static int WeaponRegistrationGet(/d; p; }' > "$tmp/scope.old"
sed -n '/^static int WeaponRegistrationScope(/,/^static int WeaponRegistrationGet(/{ /^static int WeaponRegistrationGet(/d; p; }' "$reg" > "$tmp/scope.new"
git show "$base:$reg" | sed -n '/^static int WeaponRegistrationBegin(/,/^static void WeaponRegistrationEnd(/{ /^static void WeaponRegistrationEnd(/d; p; }' > "$tmp/begin.old"
sed -n '/^static int WeaponRegistrationBegin(/,/^static void WeaponRegistrationEnd(/{ /^static void WeaponRegistrationEnd(/d; p; }' "$reg" > "$tmp/begin.new"
cp src/game/cod1_gsc/gsc_session_clock_r50.h "$tmp/"
cp tools/tests/opt_r50b_session_clock_test.c "$tmp/test.c"
test "$(git hash-object "$tmp/snapshot.inc")" = ddbbb347c2b9c064e0db8fe7cc36de548a98fe68
test "$(git hash-object "$tmp/clock.inc")" = f090d87cc755b9c6af29712bad0e5210a933c9ad
test "$(git hash-object "$tmp/item.old")" = 49376d1d38c34e5245445fa46f5f8577eede9853
test "$(git hash-object "$tmp/item.new")" = 99d989d0d5e3a58ba72affae8649b1ded7166233
test "$(git hash-object "$tmp/scope.old")" = aac2a63d0073d53798f30147a09d2da904bd5404
test "$(git hash-object "$tmp/scope.new")" = da0c48846ebcd0311edc272c91c6716f9d4cf5c0
test "$(git hash-object "$tmp/begin.old")" = f9bba43702a504d9229c997510da6ac8e10ae80a
test "$(git hash-object "$tmp/begin.new")" = d7448d270736a11c10a72fb0baf83314fc846616
test "$(git hash-object "$tmp/gsc_session_clock_r50.h")" = 7bc4c0d1d4f62bbcc45385e0dd52a7de8522f035
test "$(git hash-object "$tmp/test.c")" = 667b5af37c410f21ef738ca5076783b03b1def06
cc -std=c11 -O2 -Wall -Wextra -Werror "$tmp/test.c" -o "$tmp/test"
"$tmp/test"
