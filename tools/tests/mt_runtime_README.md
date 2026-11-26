# R30 portable worker-runtime validation

`mt_runtime_test.c` includes the actual `src/qcommon/cod1_jobs_win.inc` dispatcher
through `mt_test_runtime.h`. It does not implement a second job scheduler.

The adapter supplies pthread implementations of the small Win32 synchronization
surface the runtime uses: recursive critical sections, nonblocking acquisition,
manual/automatic reset events, thread creation/join, and atomic operations. The
CPU-discovery result is a controlled test value. GNU x86 assembly supplies real
thread-local x87/MXCSR control and exception-flag hooks.

This validates dispatcher admission, context lifetime, join, synchronization,
counter publication, serial fallback, and the numerical environment used by
portable kernel tests. It does **not** compile the native MSVC assembly or
validate Windows x86 ABI, CPU Sets/WOW64 detection, stack reservation, Windows
scheduler behavior, OpenGL context ownership, game resources, or performance.

## Running

From the repository root:

```sh
gcc -std=c11 -D_POSIX_C_SOURCE=200809L -pthread \
  -Wall -Wextra -Werror -Wno-unused-function \
  -Itools/tests/mt_include tools/tests/mt_runtime_test.c \
  -lm -o /tmp/codport_mt_runtime_test
/tmp/codport_mt_runtime_test
```

For ThreadSanitizer, add `-g -O1 -fno-omit-frame-pointer -fsanitize=thread` and
use a separate output executable. For AddressSanitizer/UndefinedBehaviorSanitizer,
use `-fsanitize=address,undefined` instead. On hosts running executables under
ptrace, LeakSanitizer cannot inspect process threads; running that executable with
`ASAN_OPTIONS=detect_leaks=0` still validates address/undefined behavior. The test
also explicitly asserts zero live thread/event handles at shutdown.

## Scenarios

- Disabled/pre-initialization execution and invalid argument boundaries.
- Actual worker participation from both the main and another submitting thread.
- A contending caller completes serially while an accepted backend batch remains
  deliberately blocked. No second worker pool is created.
- Recursive submission by the accepted caller and nested submission by workers
  cannot overwrite or wait for their own parent batch.
- Actual x87/MXCSR controls and accumulated exception flags survive joining;
  unmasked exceptions choose serial execution.
- A failed chunk still joins all other chunks before returning failure.
- Delayed worker wakes cannot access a context after its batch has completed.
- Reconfiguration drains an active batch before retiring the pool; disabling or
  requesting zero workers preserves serial results and does not change logging.
- A periodic CPU-topology refresh with unchanged settings/budget publishes only
  metadata and returns while a renderer batch is deliberately blocked.
- Shutdown prevents new admissions and joins active work before closing handles.
- Partial and complete thread-creation failures retain correct results.
- Two submitters repeatedly execute and verify disjoint outputs while the main
  thread changes settings and obtains synchronized status snapshots.

## Reuse by kernel tests

Include `mt_test_runtime.h` once in a test translation unit and use the same build
include path and pthread flags. Call `C1J_Configure(1,3)` to enable the test pool,
`C1J_Configure(0,0)` for serial execution, and `C1J_Shutdown()` at the end. Kernel
results can be compared byte-for-byte and through `C1J_FpGet`.

The test CPU count defaults to four. `mt_available_cpus`, `mt_thread_credits`
(`-1` means unlimited), `mt_pause_workers`, and `mt_workers_paused` provide
controlled failure/lifetime scenarios. They are test-adapter state, not release
runtime options.
