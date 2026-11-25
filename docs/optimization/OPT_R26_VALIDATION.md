# OPT R26 — first engine-wide optimization pass

## Scope

This source snapshot is based on release 724A (22b3a11909f903d5822485ecf7e4be06363e72e0). All 45 modified existing files (44 engine files and the Windows build script) were checked against the gameplay-tested R26 archive by Git blob SHA. The remaining original files and file modes are unchanged. No new Training-specific workarounds were introduced in this optimization pass.

The changes cover the GSC VM and scheduler, navigation, BSP59 collision queries, client movement prediction, DObj and XAnim, renderer culling and PVS, PK3 lookup, temporary memory management and sound registration. The PCM mixer experiment was rejected because it slowed down in isolated benchmarks.

## Validation

- Previous R26 regression campaign: 24 test suites (R2–R25), 31 successful jobs. Isolated function and modeled-environment tests, not a full engine integration suite. The initial R2 sanitizer run timed out; its optimized retry passed.
- User-confirmed Windows compilation and game startup.
- Training loads in approximately 54 seconds.
- Initial FPS: 83; obstacle course: 55–60 FPS; ladder minimum: 25 FPS.
- Training is fully playable; transition to us_intro works.
- Approximate earlier loading observations: 724A ~73 seconds, R8 ~59 seconds. Conditions may differ; these are not controlled benchmark results.

## Build

From an x86 Native Tools Command Prompt for Visual Studio at the repository root:

```bat
python tools\build_windows.py --output build_opt_r26
```

Use the matching x86 OpenAL library and the existing training_player.cfg profile. Keep all compiled modules from the same build. Compare performance under identical resolution, FPS cap and VSync settings, separately from diagnostic logging.

## Open issues and scope of this commit

- The ladder FPS drop is still unresolved.
- Further level loading optimization requires actual profiling.
- Save/load, the rest of the campaign and broader resource-path coverage are not yet accepted.
- Some isolated data distributions (dense navigation graphs and overlapping collision bounds) can regress. No universal speedup is claimed.
- Historical regression harnesses, logs and detailed handoffs remain in the separate R26 source archive; this commit publishes the verified engine source changes and acceptance record, not those auxiliary files. The tests have not been rerun on GitHub.
