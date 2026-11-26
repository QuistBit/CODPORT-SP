"""Compile and run the actual IBSP59 loader with a concurrent POSIX dispatcher.

Usage: python tools/tests/mt_v59_loader_test.py [--sanitize]
Requires a POSIX C11 compiler, pthreads and GNU-compatible linker wrapping.
Tests synthetic BSP data, never claims game performance or MSVC validation.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[2]
    sources = [root / "tools/tests/mt_v59_loader_test.c"]
    sources += [root / "src/qcommon/v59" / (name + ".c") for name in (
        "v59_load", "v59_curve_build", "v59_math", "v59_polylib", "v59_query", "v59_narrow")]
    flags = ["-std=c11", "-O1" if args.sanitize else "-O2", "-g", "-fno-fast-math"]
    if args.sanitize:
        flags += ["-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie"]
    with tempfile.TemporaryDirectory(prefix="mt-v59-test-") as tmp:
        exe = Path(tmp) / "mt_v59_loader_test"
        command = [os.environ.get("CC", "cc"), *flags, *map(str, sources),
                   "-Wl,--wrap=v59_curve_build,--wrap=malloc,--wrap=calloc,--wrap=free",
                   "-pthread", "-lm", "-o", str(exe)]
        subprocess.run(command, check=True, cwd=root)
        env = os.environ.copy()
        if args.sanitize:
            env.setdefault("ASAN_OPTIONS", "detect_leaks=1:halt_on_error=1")
            env.setdefault("UBSAN_OPTIONS", "halt_on_error=1:print_stacktrace=1")
        subprocess.run([str(exe)], check=True, cwd=root, env=env)


if __name__ == "__main__":
    main()
