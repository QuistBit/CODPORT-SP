#!/bin/sh
set -eu
cd "$(git rev-parse --show-toplevel)"
# Requires a full checkout containing the pinned baseline commits and cc.
for stage in r48a_array_clone r48b_array_reaches r48c_function_closure \
             r49a_clear_dormant r49b_sibling_paths r49c_disposable_knob \
             r50a_model_configstring r50b_session_clock r51a_load r51b_admission_ranges
do
    echo "=== $stage ==="
    sh "tools/tests/run_opt_${stage}_test.sh"
done
echo "R48-R51 isolated regression drivers passed"
