# OPT R39 — GSC queue admission root lookups

Base: R38 `cfaead9b5c16ffaa83519f393f4e593164ef09f6`. R37 and R38 have not yet been accepted in a native game run. R36 Hunk capacity is unchanged. This is a bounded GSC scheduling optimization, not a measured reduction of the course peak.

## Source and change

The R237 scheduler checks physical time-root membership and active binding via
r243_time_root_find and the required/physical/active predicates in
gsc_vm_r243_time_root.inc. The old queue_available calls owner_stamp,
owner_current, insertion_next twice and bucket twice on its successful path.
A fresh queue_offer makes four additional registry lookups before calling
queue_available and one final insertion claim. No callbacks or allocation
occur between these reads: task_member only scans pointer values in the
stable task directory (gsc_vm_lifetime.inc).

Collect the scalar queue view once per admission check, from the same
registered root and original predicates. No cached pointer/identity survives
the operation. Keep task membership, domain/range, existing-key identity,
capacity, and the final independently checked insertion claim unchanged.
queue_offer still calls queue_available for a fresh view at the original
admission boundary. Same writes, insertion order, diagnostics and error order.
Legacy inline mode still requires absence of a selected/registered root.
Do not optimize r237_drain_bucket_slice by caching tasks across dispatch:
callbacks can change their eligibility, directory, ownership and queue order.

The successful new selected queue offer executes 3 root lookups rather than
11 in the instrumented differential fixture; standalone successful capacity
check uses 1 rather than 6. This is a count of lookup invocations, not a
timing or FPS result. Other native guards remain in place. The helper is
private to this scheduler, not a general time-root lease.

## Validation

The original queue functions in the fixture were checked against the
fetched R38 source (ignoring comments/whitespace and reference names).
The fixture uses the new production include, minimal Runtime/Task/root
types and mirrored root predicates, task membership and insertion claim.
It exercises 28,800 configurations and 57,600 old/new call comparisons:
legacy, selected, missing/foreign root, wrong program/incarnation,
disabled/closing/teardown, missing binding, invalid membership,
existing queue identity, bucket bounds, and insertion counter exhaustion.
All return values, diagnostics, insertion counters and Task bytes match.
Strict GCC and ASan/UBSan pass.

    cc -std=c99 -O2 -Wall -Wextra -Werror tools/tests/opt_r39_queue_admission_test.c -o /tmp/r39test
    /tmp/r39test
    cc -std=c99 -O1 -g -Wall -Wextra -Werror -fsanitize=address,undefined -fno-omit-frame-pointer -no-pie tools/tests/opt_r39_queue_admission_test.c -o /tmp/r39san
    ASAN_OPTIONS=detect_leaks=0 /tmp/r39san

Local harness production include Git blob: 1aaeebd2dfc77320f4a6b1ec1be92ff692e59096.
Local harness test Git blob: d33fc6ab75b7f73f042271f8bf7fea3a7d2125da.
This is not an integrated GAME build or a full scheduler execution test.

## Remaining work

The earlier R38 audit of FaceFrame clone/fence/closure and callback checks
still needs separate active-time instrumentation. In R39 source review,
controller_storage_separate walks all program function parameter allocations
and all Task/Results allocations. These overlap checks protect separately
owned storage; no change without proving coverage across all callback
boundaries. Do not equate GSC time with exclusive bytecode execution.
Continue R40 resource first-use audit, then R41 and R42. Retain R36/R35
profilers and measure native full Training and presentation-frame traces
before acceptance. The 60 FPS and 5/2/1 ms engineering targets are not yet
verified.
