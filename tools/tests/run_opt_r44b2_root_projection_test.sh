#!/bin/sh
# Extract the exact production projection functions, not a second algorithm.
# Run from a checkout containing R44C, R44B1 and R40 history.
set -eu
cd "$(git rev-parse --show-toplevel)"
path=src/game/gsc_host/g_cod1_actor_current_root.inc
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT HUP INT TERM
awk '/^static const c1ago14Prepared_t \*ActorCurrentRootPrepared\(/ {p=1} /^static int ActorCurrentRootResource\(/ {p=0} /^static int ActorCurrentRootCollect\(/ {p=1} /^static int ActorCurrentRootFence\(/ {p=0} p' "$path" > "$tmp/r44b2_new.inc"
cc=${CC:-cc}
fixture=tools/tests/opt_r44b2_root_projection_test.c
for base in 1aca17b54f17359218939874d94b541fa497cdb0 8f92680c078b717f61aa762ef9a5c93e10eb99cb; do
    git show "$base:$path" > "$tmp/old_source"
    awk '/^static int ActorCurrentRootOrder(Visit)?\(/ {p=1} /^static int ActorCurrentRootResource\(/ {p=0} /^static int ActorCurrentRootCollect\(/ {p=1} /^static int ActorCurrentRootFence\(/ {p=0} p' "$tmp/old_source" |
     sed -e 's/C1ActorCurrentRoot/LegacyRoot/g' -e 's/ActorCurrentRootOrder/LegacyOrder/g' -e 's/ActorCurrentRootCollect/LegacyCollect/g' > "$tmp/r44b2_old.inc"
    printf 'Baseline: %s\n' "$base"
    "$cc" -std=c99 -O2 -Wall -Wextra -Werror -I "$tmp" -I src/qcommon "$fixture" -o "$tmp/test"
    "$tmp/test"
    "$cc" -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie -I "$tmp" -I src/qcommon "$fixture" -o "$tmp/test_san"
    "$tmp/test_san"
done
