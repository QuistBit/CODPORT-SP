#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
base=ef49b3abe3c1b99169356cde9a0aa1c00609848f
candidate=a8689fc5c2a1479c1739366cba404a2b2bbba6cc
p=src/game/cod1_gsc
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
git show "$base:$p/gsc_vm_r237_frame_buckets.inc" > "$tmp/old"
git show "$candidate:$p/gsc_vm_r237_frame_buckets.inc" > "$tmp/new"
git show "$candidate:$p/gsc_vm_r243_time_root.inc" > "$tmp/root"
git show "$candidate:$p/gsc_vm_r237_queue_admission.inc" > "$tmp/admit"
extract() { awk -v prefix="$1" 'index($0,prefix)==1 { active=1; found=1 } active {print; if($0=="}") exit} END {if(!found) exit 1}' "$2"; }
{
 extract "static uint64_t r237_owner_stamp(" "$tmp/root"
 extract "static int r237_owner_current(" "$tmp/root"
 extract "static int r237_bucket(" "$tmp/root"
 awk '/^typedef struct \{/ {active=1} /^static int r237_queue_available\(/ {exit} active {print}' "$tmp/admit"
 for x in old new; do extract "static int r237_drain_bucket_slice(" "$tmp/$x" | sed "s/r237_drain_bucket_slice(/${x}_drain(/"; done
 for x in old new; do extract "static int r237_frame_valid(" "$tmp/$x" | sed "s/r237_frame_valid(/${x}_valid(/"; done
} > "$tmp/r53b_extracted.h"
${CC:-cc} -std=c11 -O2 -Wall -Wextra -Werror -I"$tmp" tools/tests/opt_r53b_root_view_test.c -o "$tmp/test"
"$tmp/test"
