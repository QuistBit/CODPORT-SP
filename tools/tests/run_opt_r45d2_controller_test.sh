#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
src=src/game/gsc_host
ref=8f92680c078b717f61aa762ef9a5c93e10eb99cb
test=tools/tests/opt_r45d2_controller_test.c
git show "$ref:$src/g_cod1_actor_scripted_controller.inc" > "$tmp/r40.inc"
git show "$ref:$src/g_cod1_actor_generic_host.inc" > "$tmp/r40generic.inc"
awk '
 /^\/\* Main-thread GAME transactions/ {on=1}
 on && /^typedef struct \{$/ {copy=1}
 copy && /^static int ActorGenericNamedActive/ {exit}
 copy {print}
 ' "$tmp/r40generic.inc" > "$tmp/reference.inc"
extract() {
 awk '/^static int ActorControllerProviderDeny\(/ {on=1} /^\/\* REQUIRED REAL service/ {exit} on {print}' "$1" > "$2/gate.inc"
 awk '/^static int ActorControllerIdentity\(/ {on=1} /^static void ActorControllerEnd\(/ {exit} on {print}' "$1" > "$2/scope.inc"
 awk '/^static int ActorControllerExpectedBase\(/ {on=1} /^static int ActorControllerInitClock\(/ {exit} on {print}' "$1" > "$2/expected.inc"
}
mkdir "$tmp/old" "$tmp/new"
extract "$tmp/r40.inc" "$tmp/old"
extract "$src/g_cod1_actor_scripted_controller.inc" "$tmp/new"
test "$(git hash-object "$tmp/reference.inc")" = 34a88b8984e4e94338374f5ecb88f46a66cb9762
test "$(git hash-object "$tmp/old/gate.inc")" = de86ec558ac8e5efd77a0be14fd0284271748772
test "$(git hash-object "$tmp/old/scope.inc")" = f361404026927cea27e5f83f1c3e35f1e3069b82
test "$(git hash-object "$tmp/old/expected.inc")" = 10f933244e8dd54bc261c3aa570bf7180f8223d8
for mode in old new; do
 flags=
 if [ "$mode" = old ]; then flags=-DREFERENCE_R40; fi
 ${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror $flags -I"$tmp/$mode" -I"$tmp" -I"$src" "$test" -o "$tmp/$mode-strict"
 "$tmp/$mode-strict"
 ${CC:-cc} -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie $flags -I"$tmp/$mode" -I"$tmp" -I"$src" "$test" -o "$tmp/$mode-san"
 "$tmp/$mode-san"
done
