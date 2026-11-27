#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
src=src/game/gsc_host
ref=8f92680c078b717f61aa762ef9a5c93e10eb99cb
test=tools/tests/opt_r45d_face_callback_test.c
git show "$ref:$src/g_cod1_actor_face_frame.inc" > "$tmp/r40face.inc"
git show "$ref:$src/g_cod1_actor_generic_host.inc" > "$tmp/r40generic.inc"
git show "$ref:$src/g_cod1_face_frame_storage35.h" > "$tmp/g_cod1_face_frame_storage35.h"
awk '
 /^\/\* Main-thread GAME transactions/ {on=1}
 on && /^typedef struct \{$/ {copy=1}
 copy && /^static int ActorGenericNamedActive/ {exit}
 copy {print}
 ' "$tmp/r40generic.inc" > "$tmp/reference.inc"
extract() {
 awk '/^static int ActorFaceFrameFence\(/ {on=1} /^int G_Cod1ActorFaceClockSelect\(/ {exit} on {print}' "$1" > "$2/face_core.inc"
 awk '/^static void ActorFaceFramePoison\(/ {on=1} on {print}' "$1" > "$2/face_drain.inc"
}
mkdir "$tmp/old" "$tmp/new"
extract "$tmp/r40face.inc" "$tmp/old"
extract "$src/g_cod1_actor_face_frame.inc" "$tmp/new"
# At R45D1 the actual function bodies are byte-identical. Keep the old
# fragments separate so future production changes are still tested.
test "$(git hash-object "$tmp/old/face_core.inc")" = ca9944da0a41e954ad7599062a799125e9d6b4bc
test "$(git hash-object "$tmp/old/face_drain.inc")" = bee28e3d168aadf8cff3f3fa62250633ceb22345
test "$(git hash-object "$tmp/reference.inc")" = 34a88b8984e4e94338374f5ecb88f46a66cb9762
for mode in old new; do
 flags=
 if [ "$mode" = old ]; then flags=-DREFERENCE_R40; fi
 ${CC:-cc} -std=c99 -O2 -Wall -Wextra -Werror -Wno-misleading-indentation $flags -I"$tmp/$mode" -I"$tmp" -I"$src" "$test" -o "$tmp/$mode-strict"
 "$tmp/$mode-strict"
 ${CC:-cc} -std=c99 -O1 -g -Wall -Wextra -Werror -Wno-misleading-indentation -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie $flags -I"$tmp/$mode" -I"$tmp" -I"$src" "$test" -o "$tmp/$mode-san"
 "$tmp/$mode-san"
done
