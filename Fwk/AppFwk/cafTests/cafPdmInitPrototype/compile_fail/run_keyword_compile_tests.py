#!/usr/bin/env python3
"""Compile-time keyword validation test for issue #1202, step 1.3.

Compiles keyword_cases.cpp once with CASE=0 (all valid uses, must compile, link and run) and once
for each invalid case (must fail to compile). Prints the first diagnostic for each rejected case, so
the quality of the error messages can be compared between compilers and API candidates.

Examples:
    python run_keyword_compile_tests.py --compiler cl
    python run_keyword_compile_tests.py --compiler clang++
    python3 run_keyword_compile_tests.py --compiler g++-14
"""

import argparse
import os
import re
import subprocess
import sys
import tempfile

CASES = {
    1: "A: XML keyword with space",
    2: "A: XML keyword starting with digit",
    3: "A: XML keyword starting with xml",
    4: "A: empty XML keyword",
    5: "A: scriptable keyword starting with lowercase",
    6: "A: scriptable keyword with three uppercase letters",
    7: "A: separate script keyword with underscore",
    8: "A: keyword is not a constant expression",
    9: "A: scriptable keyword with invalid character",
    10: "A: legacy script keyword with underscore",
    11: "A: legacy script keyword starting with xml",
    21: "B: XML keyword with space",
    22: "B: XML keyword starting with digit",
    23: "B: XML keyword starting with xml",
    24: "B: empty XML keyword",
    25: "B: scriptable keyword starting with lowercase",
    26: "B: scriptable keyword with three uppercase letters",
    27: "B: separate script keyword with underscore",
    28: "B: scriptable check after other builder calls",
    30: "B: legacy script keyword with underscore",
}


def is_msvc(compiler):
    name = os.path.basename(compiler).lower()
    return name in ("cl", "cl.exe", "clang-cl", "clang-cl.exe")


def command(compiler, source, include_dir, case, output_dir):
    if is_msvc(compiler):
        cmd = [compiler, "/nologo", "/std:c++latest", "/EHsc", "/W4", "/I", include_dir, f"/DCASE={case}"]
        if case == 0:
            return cmd + [source, f"/Fo{output_dir}{os.sep}", f"/Fe{os.path.join(output_dir, 'keyword_cases.exe')}"]  # codespell:ignore
        return cmd + ["/c", source, f"/Fo{output_dir}{os.sep}"]  # codespell:ignore

    cmd = [compiler, "-std=c++23", "-Wall", "-Wextra", "-I", include_dir, f"-DCASE={case}"]
    if case == 0:
        return cmd + [source, "-o", os.path.join(output_dir, "keyword_cases.exe")]
    return cmd + ["-fsyntax-only", source]


REASONS = [
    "keywordIsEmpty",
    "keywordFirstCharacterIsInvalid",
    "keywordStartsWithXml",
    "keywordContainsInvalidCharacter",
    "scriptKeywordIsNotCamelCase",
    "scriptKeywordMustStartWithUppercaseLetter",
    "static assertion failed",
    "static_assert failed",
    "is not a constant expression",
]


def first_diagnostic(output):
    """Return the first error line, and the reason reported by the compiler if it is on another line."""
    lines = [line.strip() for line in output.splitlines()]
    errors = [line for line in lines if re.search(r"\berror\b", line)]
    first = errors[0] if errors else "(no error line found)"

    # Strip the path, the interesting part is the message
    first = re.sub(r"^.*keyword_cases\.cpp", "keyword_cases.cpp", first)

    for reason in REASONS[:6]:
        if any(reason in line for line in lines):
            if reason not in first:
                first += f"\n        reason reported in note: {reason}"
            break
    return first


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--compiler", required=True)
    args = parser.parse_args()

    this_dir = os.path.dirname(os.path.abspath(__file__))
    source = os.path.join(this_dir, "keyword_cases.cpp")
    include_dir = os.path.dirname(this_dir)

    version_flag = [] if is_msvc(args.compiler) else ["--version"]
    version = subprocess.run([args.compiler] + version_flag, capture_output=True, text=True)
    print((version.stderr + version.stdout).strip().splitlines()[0])

    failures = 0
    with tempfile.TemporaryDirectory() as output_dir:
        result = subprocess.run(command(args.compiler, source, include_dir, 0, output_dir), capture_output=True, text=True)
        if result.returncode != 0:
            print("FAIL  valid cases did not compile")
            print(result.stdout + result.stderr)
            failures += 1
        else:
            run = subprocess.run([os.path.join(output_dir, "keyword_cases.exe")])
            ok = run.returncode == 0
            failures += 0 if ok else 1
            print(f"{'OK  ' if ok else 'FAIL'}  valid cases compile and run")

        for case, description in CASES.items():
            result = subprocess.run(
                command(args.compiler, source, include_dir, case, output_dir), capture_output=True, text=True
            )
            rejected = result.returncode != 0
            failures += 0 if rejected else 1
            print(f"{'OK  ' if rejected else 'FAIL'}  case {case:2}: {description}")
            if rejected:
                print(f"        {first_diagnostic(result.stdout + result.stderr)}")

    print(f"{failures} failure(s)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
