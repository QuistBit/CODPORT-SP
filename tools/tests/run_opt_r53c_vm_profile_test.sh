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
profile=5b8fd3be7828d82cd1301bdaf59c3eb522960420
git show "$profile:$p/gsc_vm_r237_frame_buckets.inc" > "$tmp/profile"
git show "$profile:$p/gsc_r237_profile53.h" > "$tmp/types"
{
 awk '/^enum \{/ {active=1} /^int gsc_r237_frame_phase_profile53/ {exit} active {print}' "$tmp/types"
 grep "^static uint64_t r237_profile_begin53(" "$tmp/profile"
 extract "static void r237_profile_end53(" "$tmp/profile"
 cat <<'MOCK'
static int r237_dispatch_task53(GscRuntime *r,Task *chosen,int *budget,uint64_t stamp,GscR237Profile53 *p){
 (void)p;return r237_dispatch_task(r,chosen,budget,stamp);
}
MOCK
 extract "static int r237_drain_bucket_slice53(" "$tmp/profile"
} > "$tmp/profile_scan.h"
# GCC/Clang: CC=clang bash this script. Override CFLAGS for sanitizer runs.
read -r -a flags <<< "${CFLAGS:--std=c11 -O2 -Wall -Wextra -Werror}"
"${CC:-cc}" "${flags[@]}" -I"$tmp" tools/tests/opt_r53c_vm_profile_test.c -o "$tmp/test"
"$tmp/test"
