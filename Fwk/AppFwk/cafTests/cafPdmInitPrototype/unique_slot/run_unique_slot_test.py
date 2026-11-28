#!/usr/bin/env python3
"""Verify whether a lambda used as default template argument gives one static slot per call site.

Prototype for issue #1202, step 1.2. Builds main.cpp, unit_one.cpp and unit_two.cpp and runs the
result. The program returns 0 only if all call sites in both translation units get distinct slots.

    python run_unique_slot_test.py --compiler cl
    python3 run_unique_slot_test.py --compiler clang++-19
"""

import argparse
import os
import subprocess
import sys
import tempfile


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--compiler", required=True)
    args = parser.parse_args()

    this_dir = os.path.dirname(os.path.abspath(__file__))
    sources = [os.path.join(this_dir, name) for name in ("main.cpp", "unit_one.cpp", "unit_two.cpp")]
    msvc = os.path.basename(args.compiler).lower() in ("cl", "cl.exe")

    result = 0
    for optimization in (("/Od", "/O2") if msvc else ("-O0", "-O2")):
        with tempfile.TemporaryDirectory() as output_dir:
            exe = os.path.join(output_dir, "unique_slot.exe")
            if msvc:
                cmd = [args.compiler, "/nologo", "/std:c++latest", "/EHsc", optimization] + sources
                cmd += [f"/Fo{output_dir}{os.sep}", f"/Fe{exe}"]  # codespell:ignore
            else:
                cmd = [args.compiler, "-std=c++23", optimization] + sources + ["-o", exe]

            build = subprocess.run(cmd, capture_output=True, text=True)
            if build.returncode != 0:
                print(build.stdout + build.stderr)
                return 1

            print(f"--- {args.compiler} {optimization}")
            sys.stdout.flush()
            result |= subprocess.run([exe]).returncode

    return result


if __name__ == "__main__":
    sys.exit(main())
