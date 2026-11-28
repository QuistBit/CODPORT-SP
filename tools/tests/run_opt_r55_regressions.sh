#!/usr/bin/env bash
set -euo pipefail
cd "$(git rev-parse --show-toplevel)"
bash tools/tests/run_opt_r55a_source_test.sh
bash tools/tests/run_opt_r55b_breakdown_test.sh
bash tools/tests/run_opt_r55c1_readonly_path_test.sh
bash tools/tests/run_opt_r55c2a_state_reserve_test.sh
bash tools/tests/run_opt_r55c2b_state_lookup_test.sh
bash tools/tests/run_opt_r55c3a_cow_lookup_test.sh
bash tools/tests/run_opt_r55c3b_array_lookup_test.sh
bash tools/tests/run_opt_r55c4_local_lookup_test.sh
