# OPT R27 — coarse map load timing

Base: accepted R26 master 8cc16d1cd051bc6d33103ed2d801a5ad18b6d5d9.

## What changed
Opt-in `com_cod1LoadProfile` (default 0, CVAR_TEMP). Only three synchronous
engine-side load scopes are instrumented. No game, renderer, VM ABI, resource
or gameplay changes. No per-frame instrumentation. The flag is sampled at
scope entry. Disabled scopes take no additional clock samples or print calls.
Stage timestamps use unscaled Sys_Milliseconds and unsigned subtraction.
Samples are buffered on the stack and printed after each completed scope;
an aborted scope need not have a report. No persistent profile state.

- server_spawn: shutdown, client loading screen, client/hunk teardown,
  server setup and FS restart, collision load, game init, settling/baseline,
  reconnect, final frame and server info.
- client_load: event loop and client flush, cgame initialization (inclusive),
  pure checksums and packets; download-restart/abandoned scopes are excluded.
- cgame_init: reset/VM creation/ABI, CG_INIT and key bindings, renderer
  EndRegistration, memory touch/hunk usage.

The client_load and cgame_init scopes overlap: do not add their totals.
Parent time includes the cost of child diagnostic output. Server and client
scopes do not capture the gap between server spawn and client loading,
map-command preflight or time from packets to first playable frame.
The report is wall time, not CPU usage; the timer resolution is platform
dependent. No claim is made about the cause of the 54-second observation.

## Validation and next step
From an x86 Native Tools Command Prompt for Visual Studio:
```bat
python tools\build_windows.py --output build_opt_r27
```
Use matching modules and the same training_player.cfg, resolution,
VSync, FPS cap, resource paths and storage as R26. At main menu console:
```text
set com_cod1LoadProfile 1
spmap training
```
Collect rtcwconsole.log in its entirety; search for [OPT_LOAD] and the
existing CL_InitCGame line. Repeat on a fresh process with the same settings;
separate first-load and repeat-load observations. Then set the flag to 0
and repeat Training, including the transition to us_intro. There must be
no [OPT_LOAD] lines emitted with the flag off. Do not compare diagnostic
runs to uninstrumented FPS as a controlled performance benchmark.

No Windows build or gameplay run has been performed for R27 here.
Source changes are insertion-only, with exact anchors and reversal checked.
Next: R28 frame profiling, inspect existing com_speeds and R241 trace before
adding anything. For R27 follow-up, obtain real load timings first; if the
largest bucket is CG_INIT or game init, instrument its internal phases
rather than guessing which resource or subsystem is expensive.

## Isolated helper regression
The exact helper header (Git blob becf91c1b03df28f7466f7c2292260e07a49962c)
was tested against a stub clock, cvar and logger with GCC-compatible C99:
```sh
cc -std=c99 -Wall -Wextra -Werror -O2 -Isrc/qcommon tools/tests/opt_r27_load_profile_test.c -o /tmp/r27_test
/tmp/r27_test
cc -std=c99 -Wall -Wextra -Werror -fsanitize=undefined -g -Isrc/qcommon tools/tests/opt_r27_load_profile_test.c -o /tmp/r27_ubsan
/tmp/r27_ubsan
```
Both runs passed: disabled mode, exact log format and stage totals,
nested independent scopes, unsigned timer wraparound, array bound,
and reinitialization. This is not a full engine or VM integration test.
