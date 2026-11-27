#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
src=src/game/gsc_host
awk '/^static int ActorLocomotionKernel46\(/ {on=1} /^static int ActorLocomotionFrameAdvance\(/ {exit} on {print}' "$src/g_cod1_actor_locomotion_resources.inc" > "$tmp/kernel_profile.inc"
test -s "$tmp/kernel_profile.inc"
for target in portable winbranch; do
 defines=
 if [ "$target" = winbranch ]; then defines='-D_WIN32 -D_MSC_VER=1930 -D_M_IX86'; fi
 for mode in strict san; do
  flags=-O2
  if [ "$mode" = san ]; then flags='-O1 -g -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie'; fi
  ${CC:-cc} -std=c99 -Wall -Wextra -Werror $defines $flags -I"$src" -I"$tmp" tools/tests/opt_r46c_course_profile_test.c -o "$tmp/$target-$mode"
  "$tmp/$target-$mode"
 done
done
