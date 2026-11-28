#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
base=a5b86817728935762d076ce866f9a9dbbd015c78
candidate=2181fe27827eaef5d664b9316bc3dc997eb7b815
p=src/game/cod1_gsc
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
git show "$base:$p/gsc_session_r237_frame_buckets.inc" > "$tmp/session_old"
git show "$candidate:$p/gsc_session_r237_frame_buckets.inc" > "$tmp/session_new"
git show "$base:$p/gsc_vm_r237_frame_buckets.inc" > "$tmp/vm_old"
git show "$candidate:$p/gsc_vm_r237_frame_buckets.inc" > "$tmp/vm_new"
extract() { awk -v start="$1" -v stop="$2" 'index($0,stop)==1 {exit} index($0,start)==1 {active=1;found=1} active {print} END {if(!found) exit 1}' "$3"; }
extract "static int session_r237_phase53(" "int gsc_session_r237_frame_phase_slice(" "$tmp/session_old" > "$tmp/old.inc"
extract "static uint64_t session_profile_begin53D(" "int gsc_session_r237_frame_phase_slice(" "$tmp/session_new" > "$tmp/new.inc"
for v in old new; do extract "static int r237_dispatch_task53(" "static int r237_dispatch_task(GscRuntime" "$tmp/vm_$v" > "$tmp/dispatch_$v.inc"; done
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror}"
"${CC:-cc}" "${flags[@]}" -I"$tmp" tools/tests/opt_r53d_session_test.c -o "$tmp/session"
"$tmp/session"
"${CC:-cc}" "${flags[@]}" -I"$tmp" tools/tests/opt_r53d_task_guard_test.c -o "$tmp/guard"
"$tmp/guard"
