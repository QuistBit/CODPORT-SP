#!/usr/bin/env python3
"""Compile real geometry kernels with the portable adapter for the C1J pool.

Requires a GNU-compatible x86 C compiler, pthreads, and libm. This verifies
portable source behavior, not MSVC code generation, Win32 ABI, or gameplay.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true", help="ASan and UBSan; leak detection is unavailable under this host's ptrace")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    with tempfile.TemporaryDirectory(prefix="mt-skin-") as output:
        executable = Path(output) / "geometry"
        command = [os.environ.get("CC", "cc"), "-std=c11", "-O1" if args.sanitize else "-O2",
                   "-frounding-math", "-fno-fast-math", "-fno-strict-aliasing",
                   "-ffunction-sections", "-fdata-sections", "-D_POSIX_C_SOURCE=200809L",
                   "-pthread", "-Itools/tests/mt_include", "-Wall", "-Wextra",
                   "-Wno-unused-function", "-Wno-misleading-indentation", "-Wno-sign-compare",
                   "-Wno-unused-parameter", "-Wno-missing-braces", "-Wno-array-bounds",
                   "tools/tests/mt_skin_geometry_test.c", "src/game/q_math.c",
                   "-Wl,--gc-sections", "-lm", "-o", str(executable)]
        if args.sanitize:
            command += ["-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
        subprocess.run(command, cwd=root, check=True)
        environment = os.environ.copy()
        if args.sanitize:
            environment["ASAN_OPTIONS"] = "detect_leaks=0"
        subprocess.run([str(executable)], cwd=root, env=environment, check=True)


if __name__ == "__main__":
    main()
