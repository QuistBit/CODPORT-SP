"""Build and run actual lighting kernels with the production job dispatcher.

The pthread Win32 adapter verifies scheduling and numerical parity on this
host. It does not validate MSVC x86 code generation, drivers or game FPS.
"""
import argparse
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--sanitize", action="store_true")
    parser.add_argument("--simd", action="store_true", help="Exercise the real SSE entity-light path on an SSE4.1-capable test host")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="cod1-mt-light-") as directory:
        binary = Path(directory) / "mt_render_light_test"
        command = ["cc", "-std=c11", "-D_POSIX_C_SOURCE=200809L", "-pthread",
                   "-frounding-math", "-ffp-contract=off", "-Wall", "-Wextra",
                   "-Wno-unused-function", "-Wno-misleading-indentation",
                   "-I" + str(ROOT / "tools/tests/mt_include"),
                   str(ROOT / "tools/tests/mt_render_light_test.c"), "-lm", "-o", str(binary)]
        command[1:1] = (["-O1", "-g", "-fsanitize=address,undefined", "-fno-omit-frame-pointer"]
                        if args.sanitize else ["-O2"])
        if args.simd:
            command[1:1] = ["-DMT_TEST_SIMD", "-msse4.1"]
        subprocess.run(command, check=True, cwd=ROOT)
        environment = os.environ.copy()
        if args.sanitize:
            # LSan is unavailable under this host's ptrace supervision.
            environment["ASAN_OPTIONS"] = "detect_leaks=0:halt_on_error=1"
            environment["UBSAN_OPTIONS"] = "halt_on_error=1"
        subprocess.run([str(binary)], check=True, cwd=ROOT, env=environment, timeout=45)


if __name__ == "__main__":
    main()
