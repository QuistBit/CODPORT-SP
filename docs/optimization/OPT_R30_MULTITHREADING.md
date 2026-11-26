# OPT R30 — shared CPU workers and single/multi launch modes

Parent: R29 `69075965124c3cc924b50642d5117f721d5c0cd8` from `optimization/phase2`.
Accepted master at the start of this work: R26 `8cc16d1cd051bc6d33103ed2d801a5ad18b6d5d9`.
Development branch: `optimization/threading-r30`.

R30 implements the requested expansion of parallel computation while keeping a
sequential launch mode for comparison. It follows the user's request to proceed
beyond the pending R27–R29 measurements. Those measurements remain useful: this
patch does not establish the cause of the Training ladder slowdown or a measured
performance improvement.

## What changed

| Area | R30 behavior | Ownership retained by the caller |
| --- | --- | --- |
| Shared worker pool | Accepts synchronous batches from the main/frontend and renderer threads | One worker budget, configuration and shutdown |
| Assembled actors | One flattened vertex batch across model parts, instead of separate part joins | Pose preparation, bounds, tags and publication |
| Native first-person hands and weapon | Independent skinning across both captured parts | Animation selection, poses, tags and arena ownership |
| Pilot actors and MG42 | Batches pilot actor parts and static/animated turret captures | Allocation, ordered bounds and failure rollback |
| Entity lighting | Renderer can actually use the pool; cvar/context settings are captured before dispatch | Immutable frame/surface lifetime and result publication |
| Static model colors | Rebuilds independent cached vertex colors concurrently | Cache keys, revisions, counters and buffer swap |
| Static model baking | Transforms and shades eligible vertices concurrently | Visibility fallback, bounds and engine allocation |
| Dynamic per-light projection | Projects eligible vertex ranges concurrently | Triangle selection, tessellation state and GL commands |
| IBSP59 collision loading | Builds independent type-1 triangle meshes in batches of up to 64 | Curve construction, allocation ownership merge and error order |

The earlier pool rejected non-owner submissions. Consequently, lighting called
from the real render thread ran serially even when `com_multithreading=1`. R30
removes that restriction with synchronized admission. Engine `r_smp` defaults to
zero, but the existing packaged configuration and Training launcher select one;
the actual renderer state must be checked rather than inferred from either default.

The native pool remains Windows x86-specific. Other existing platform backends
remain serial. Small jobs stay serial to avoid paying dispatch/join overhead:
lighting uses 256-vertex chunks for eligible surfaces of at least 512 vertices,
and captured geometry uses 512-vertex chunks across sufficiently large spans.
Automatic worker count uses the existing effective CPU availability calculation,
reserving one logical CPU and capping the pool at 64 workers. This is one shared
pool, not an additional full pool for every subsystem.

## Concurrency and numerical contract

An accepted submitter owns admission until all chunks complete and its context
is retired. A competing frontend/backend submission executes its own work
serially without waiting for that admission. Recursive submissions also take a
serial path. Workers claim and complete chunks under a short separate lock;
their calculations run outside it. This avoids overwriting the shared batch or
waiting for a parent batch from within its callback.

Configuration and shutdown drain accepted work before retiring worker handles.
An unchanged periodic CPU-budget refresh only updates status metadata; it does
not wait for a rendering batch. The existing renderer shutdown fence completes
render commands, including their synchronous worker joins, before freeing the
referenced model/light resources.

Callbacks read immutable source/pose/light snapshots and write disjoint output
ranges. They do not call GL, the engine filesystem, GSC, engine allocators or
mutable visibility/cache routines. The collision loader uses private CRT
allocation lists and a separate local `setjmp` error boundary per job; ownership
is transferred after join. Curves retain the legacy serial builder. The earliest
reported mesh error is selected in map order, including partial dispatch and OOM.

The existing x87/MXCSR control and exception-flag handling is preserved. Unmasked
exceptions select the original serial route. Failed geometry/lighting batches
restore the caller's FP state before replaying the ordered fallback; partial
buffers are not published. Bounds, tags, cache swaps and global counters remain
on the submitting thread. Entity lighting retains the existing four-vertex SIMD
kernel, with chunk boundaries divisible by four.

Large actor pose arrays live on the capture caller's stack, not on the 256 KiB
worker stack. The existing Windows executable reserves an 8 MiB main stack.
This layout still requires the native build/game validation below.

## What remains sequential

GSC execution, ordered entity/world mutation, server movement and physics,
client prediction, visibility callbacks, GL submission, legacy curve building
and resource publication keep their existing ownership and ordering. R30 does
not introduce concurrent execution of arbitrary script `thread` constructs.

Parallelizing those stateful stages requires explicit read snapshots, separate
write sets and deterministic merging of dependent results. Their absence cannot
be solved by wrapping the current entity loop in `ParallelFor`. Real R27/R29
captures should identify which further stage justifies that larger refactor.

## Build once, run both modes

Use an **x86 Native Tools Command Prompt for Visual Studio**, with MSVC,
Windows SDK and Python available. Build all matching modules into a new folder:

```bat
python tools\build_windows.py --output build_opt_r30
```

Provide the 32-bit OpenAL DLL either through `--openal "C:\path\soft_oal.dll"`
at build time or by copying it into `build_opt_r30\bin`. The build script now
copies the two launchers and `THREADING-MODES.md` into the build directory.

```bat
build_opt_r30\Play-Training-Single.cmd "C:\Games\Call of Duty"
build_opt_r30\Play-Training-Multi.cmd "C:\Games\Call of Duty"
```

Without a game path, each launcher opens the existing `CoDSP.exe` picker.
The equivalent Python commands are:

```bat
python tools\run_training.py --build build_opt_r30 --game "C:\Games\Call of Duty" --thread-mode single
python tools\run_training.py --build build_opt_r30 --game "C:\Games\Call of Duty" --thread-mode multi
```

| Setting | Single | Multi |
| --- | ---: | ---: |
| `com_multithreading` | 0 | 1 |
| `com_workerThreads` | 0 | -1 (automatic) |
| `r_smp` | 0 | 1 |
| `com_asyncLog` | 1 | 1 |

Default launch mode remains `multi`. Command-line settings are reapplied after
configuration loading, so single mode overrides packaged `r_smp=1` before
renderer initialization. `training_player.cfg` does not override these settings.
Close the game completely between modes; this is a startup choice, not a live
renderer-context switch.

Both modes retain the same profile, assets, resolution, VSync setting and FPS
cap. Async logging now depends on `com_asyncLog`, independently of compute-pool
enablement. The existing Training config sets `logfile=1`; startup still uses
the launcher's `logfile=2` until that config executes. Audio/log service threads
can exist in single mode: the mode describes game/render computation, not the
number of all OS threads in the process.

The frozen `packaging/windows/package_release.py` continues to qualify the old
724A binary hashes. R30 is a source change; no replacement 724A archive or newly
qualified Windows executable has been produced by this work.

## Evening comparison and diagnostics

1. Start a new Training game in each mode with the same build and settings.
   Compare load time and the same viewpoints/actions: start, recruits at the
   wire, ladder approach/ascent, first-person hands/weapons and shooting.
2. Run `mt_status` after loading and again after gameplay. In single mode expect
   `enabled=0`, zero compute workers in a fresh process and renderer `active=0`.
   In multi mode verify actual renderer `active=1`; a requested value of one is
   not proof that renderer startup succeeded.
3. Compare counter differences: increasing `workerChunks` proves worker work.
   `created` alone only proves threads were created. `nonOwnerBatches` counts
   accepted renderer/other non-owner submissions. `contentionSerial` reports
   serial fallback when the shared pool was busy, and `nestedSerial` reports
   recursive submitter fallback; neither is automatically an error.
4. Save each `profile\main\rtcwconsole.log` before the next run. Finish Training
   and check the `us_intro` transition, then close the game normally to exercise
   shutdown. Note any animation, lighting, collision or sound regression.

The launcher retains **60 FPS**. Faster work can be hidden by this cap. Use the
same cap/VSync for a controlled comparison; if deliberately running uncapped,
apply that choice identically in both sessions. Equal capped FPS does not prove
the workers are unused. Compare slow sections and frame consistency as well.

R241/R29 detailed frame capture explicitly refuses actual SMP rendering. Use
single mode for the pending detailed R29 capture and keep tracing off for the
ordinary mode comparison. Do not present an R241 capture as a measurement of
full multi mode. R27 load profiling can still supply load-phase evidence, but
profiled and unprofiled timings should be kept distinct.

## Validation performed

The portable runtime adapter compiles the actual production dispatcher with
pthread implementations of its Win32 synchronization calls and real GNU x86
x87/MXCSR hooks. CPU discovery is controlled test input. Kernel tests compare
sequential and concurrent outputs; they do not substitute a different scheduler.
The separate IBSP59 harness uses a synchronous pthread dispatcher to exercise
the optional loader callback contract, including declined/partial dispatch.

| Check | Result in this environment |
| --- | --- |
| Production dispatcher | 10 scenario groups passed under strict GCC, ThreadSanitizer and ASan/UBSan |
| Geometry | 16 suites passed: four kernel arrangements × four rounding modes; bytes, metadata, bounds, FP flags, error replay and arena rollback |
| Lighting | Four rounding modes; colors/alpha, geometry/UVs, projection/clip, FP flags, malformed inputs and disabled-pool path; normal and ASan/UBSan passed |
| Existing SIMD entity-light kernel | Normal and ASan/UBSan runs passed through a test-only MSVC intrinsic spelling adapter |
| IBSP59 loader/query | 73 dispatches, 2,243 callbacks, peak four simultaneous callbacks; exact geometry/query records, serial curves, malformed inputs, partial dispatch and 13 worker-only allocation failures passed |
| Python launch/profile tests | All 12 tests passed: four mode tests plus eight existing R28/R29 analyzer tests |
| Integration review | Renderer teardown, immutable settings, ownership, header recipe and command-line precedence reviewed; no concrete blocker found |

Commands from the repository root:

```sh
gcc -std=c11 -D_POSIX_C_SOURCE=200809L -pthread \
  -Wall -Wextra -Werror -Wno-unused-function \
  -Itools/tests/mt_include tools/tests/mt_runtime_test.c \
  -lm -o /tmp/codport_mt_runtime_test
/tmp/codport_mt_runtime_test
python tools/tests/mt_skin_geometry_test.py
python tools/tests/mt_skin_geometry_test.py --sanitize
python tools/tests/mt_render_light_test.py
python tools/tests/mt_render_light_test.py --sanitize
python tools/tests/mt_render_light_test.py --simd
python tools/tests/mt_render_light_test.py --simd --sanitize
python tools/tests/mt_v59_loader_test.py
ASAN_OPTIONS=detect_leaks=0:halt_on_error=1 python tools/tests/mt_v59_loader_test.py --sanitize
PYTHONPATH=tools python -m unittest discover -s tools/tests -p 'test_*.py' -v
```

Runtime sanitizer flags and scenario details are in
[`mt_runtime_README.md`](../../tools/tests/mt_runtime_README.md). LeakSanitizer
cannot inspect threads under this host's ptrace supervision, so it was disabled;
address/undefined-behavior checks remained active, with explicit live-handle and
loader-allocation assertions. The geometry harness emits an existing warning
from unused legacy `Q_rsqrt` code when compiling `q_math.c` on a 64-bit host;
that discarded routine is not one of the tested kernels.

These checks do **not** validate native MSVC x86 code generation/assembly,
Windows ABI/CPU discovery, OpenGL context transfers, the complete renderer
wrappers with retail resources, or gameplay. A native Windows build and the
comparison above are still required. No FPS or loading-time gain is claimed.
