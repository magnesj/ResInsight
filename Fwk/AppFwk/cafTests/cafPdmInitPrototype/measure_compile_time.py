#!/usr/bin/env python3
"""Measure compile time and compiler memory for single translation units.

Tool for issue #1202. The compile command of each selected source file is read from the Ninja build
directory and executed directly, so the measurement is independent of the build cache, of Ninja and
of other targets. Object files are written to a temporary folder, the build directory is not changed.

Every file is compiled --runs times in sequence. The table shows the median and the minimum wall-clock
time, and the peak memory of the compiler process.

Example:
    python measure_compile_time.py --build-dir build --runs 5 bench_mixed bench_scriptable

A file is selected if its path contains one of the given texts.
"""

import argparse
import json
import os
import re
import statistics
import subprocess
import sys
import tempfile
import time


def find_ninja(build_dir):
    with open(os.path.join(build_dir, "CMakeCache.txt"), encoding="utf-8", errors="replace") as cache:
        for line in cache:
            if line.startswith("CMAKE_MAKE_PROGRAM:"):
                return line.split("=", 1)[1].strip()
    return "ninja"


def compile_commands(build_dir, ninja):
    output = subprocess.run([ninja, "-C", build_dir, "-t", "compdb"], capture_output=True, text=True, check=True).stdout
    return json.loads(output)


def outputs_depending_on(build_dir, ninja, header):
    """Return the object files with a recorded dependency on a header with the given file name."""
    output = subprocess.run([ninja, "-C", build_dir, "-t", "deps"], capture_output=True, text=True, check=True).stdout
    outputs = set()
    current = None
    for line in output.splitlines():
        if line and not line.startswith(" "):
            current = line.split(":", 1)[0].replace("\\", "/")
        elif current and line.strip().replace("\\", "/").endswith("/" + header):
            outputs.add(current)
    return outputs


def outputs_depending_on(build_dir, ninja, header):
    """Return the object files with a recorded dependency on a header with the given file name."""
    output = subprocess.run([ninja, "-C", build_dir, "-t", "deps"], capture_output=True, text=True, check=True).stdout
    outputs = set()
    current = None
    for line in output.splitlines():
        if line and not line.startswith(" "):
            current = line.split(":", 1)[0].replace("\\", "/")
        elif current and line.strip().replace("\\", "/").endswith("/" + header):
            outputs.add(current)
    return outputs


def prepare_command(command, output_dir):
    """Remove the compiler launcher and redirect all output files to output_dir."""
    # Compiler launcher, for example buildcache or ccache
    command = re.sub(r"^\S*(buildcache|ccache|sccache)\S*\s+", "", command, flags=re.IGNORECASE)

    obj = os.path.join(output_dir, "bench.obj")
    if re.search(r"(^|\s)/Fo\S+", command):  # codespell:ignore
        # MSVC. /MP is removed to make sure that the compilation runs in the measured process.
        command = re.sub(r"(^|\s)/Fo\S+", lambda m: f"{m.group(1)}/Fo{obj}", command)  # codespell:ignore
        command = re.sub(r"(^|\s)/Fd\S+", lambda m: f"{m.group(1)}/Fd{output_dir}{os.sep}", command)
        command = re.sub(r"(^|\s)/showIncludes", r"\1", command)
        command = re.sub(r"(^|\s)/MP\S*", r"\1", command)
    else:
        command = re.sub(r"(^|\s)-o \S+", lambda m: f"{m.group(1)}-o {obj}", command)
        command = re.sub(r"(^|\s)-MD -MT \S+ -MF \S+", r"\1", command)
    return command


def run_windows(command, cwd):
    import ctypes
    from ctypes import wintypes

    class ProcessMemoryCounters(ctypes.Structure):
        _fields_ = [
            ("cb", wintypes.DWORD),
            ("PageFaultCount", wintypes.DWORD),
            ("PeakWorkingSetSize", ctypes.c_size_t),
            ("WorkingSetSize", ctypes.c_size_t),
            ("QuotaPeakPagedPoolUsage", ctypes.c_size_t),
            ("QuotaPagedPoolUsage", ctypes.c_size_t),
            ("QuotaPeakNonPagedPoolUsage", ctypes.c_size_t),
            ("QuotaNonPagedPoolUsage", ctypes.c_size_t),
            ("PagefileUsage", ctypes.c_size_t),
            ("PeakPagefileUsage", ctypes.c_size_t),
        ]

    start = time.perf_counter()
    process = subprocess.Popen(command, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    output, _ = process.communicate()
    elapsed = time.perf_counter() - start

    counters = ProcessMemoryCounters()
    counters.cb = ctypes.sizeof(counters)
    get_info = ctypes.windll.psapi.GetProcessMemoryInfo
    get_info.argtypes = [wintypes.HANDLE, ctypes.POINTER(ProcessMemoryCounters), wintypes.DWORD]
    get_info(int(process._handle), ctypes.byref(counters), counters.cb)

    return process.returncode, elapsed, counters.PeakWorkingSetSize, output


def run_posix(command, cwd):
    import shlex

    start = time.perf_counter()
    process = subprocess.Popen(shlex.split(command), cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
    output = process.stdout.read()
    _, status, usage = os.wait4(process.pid, 0)
    elapsed = time.perf_counter() - start
    process.returncode = os.waitstatus_to_exitcode(status)

    # ru_maxrss is in kilobytes on Linux
    return process.returncode, elapsed, usage.ru_maxrss * 1024, output


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--build-dir", default="build")
    parser.add_argument("--runs", type=int, default=5)
    parser.add_argument("--baseline", help="Text identifying the file used as reference for the ratio column")
    parser.add_argument("--depends-on", help="Select all files including a header with this file name")
    parser.add_argument("--sample", type=int, help="Measure a random sample of the selected files")
    parser.add_argument("--seed", type=int, default=1202, help="Seed for the random sample")
    parser.add_argument("filters", nargs="*", help="Select files with a path containing one of these texts")
    args = parser.parse_args()

    build_dir = os.path.abspath(args.build_dir)
    ninja = find_ninja(build_dir)
    run = run_windows if os.name == "nt" else run_posix

    dependents = outputs_depending_on(build_dir, ninja, args.depends_on) if args.depends_on else None

    selected = {}
    for entry in compile_commands(build_dir, ninja):
        path = entry["file"].replace("\\", "/")
        if not path.endswith((".cpp", ".cc", ".cxx")):
            continue
        if dependents is not None and entry.get("output", "").replace("\\", "/") not in dependents:
            continue
        if args.filters and not any(text in path for text in args.filters):
            continue
        selected.setdefault(path, entry)

    if not selected:
        print("No files selected")
        return 1

    selected_count = len(selected)
    if args.sample and args.sample < selected_count:
        import random

        names = random.Random(args.seed).sample(sorted(selected), args.sample)
        selected = {name: selected[name] for name in names}

    results = []
    with tempfile.TemporaryDirectory() as output_dir:
        for path, entry in sorted(selected.items()):
            command = prepare_command(entry["command"], output_dir)
            times = []
            memory = []
            for _ in range(args.runs):
                returncode, elapsed, peak, output = run(command, entry["directory"])
                if returncode != 0:
                    print(f"Compilation failed: {path}\n{output}")
                    return 1
                times.append(elapsed)
                memory.append(peak)
            results.append((os.path.basename(path), statistics.median(times), min(times), max(memory)))
            print(f"measured {os.path.basename(path)}", file=sys.stderr)

    reference = None
    if args.baseline:
        reference = next((r for r in results if args.baseline in r[0]), None)

    print(f"\nRuns per file: {args.runs}\n")
    header = "| File | Median (ms) | Min (ms) | Peak memory (MB) |"
    separator = "|---|---:|---:|---:|"
    if reference:
        header += " Time ratio | Memory ratio |"
        separator += "---:|---:|"
    print(header)
    print(separator)
    for name, median, minimum, peak in results:
        row = f"| {name} | {median * 1000:.0f} | {minimum * 1000:.0f} | {peak / 1024 / 1024:.0f} |"
        if reference:
            row += f" {median / reference[1]:.2f} | {peak / reference[3]:.2f} |"
        print(row)

    if args.depends_on:
        mean = statistics.mean(r[1] for r in results)
        print(f"\nFiles depending on {args.depends_on}: {selected_count}")
        print(f"Measured files: {len(results)}, mean compile time {mean * 1000:.0f} ms")
        print(f"Estimated compile time for all dependent files: {mean * selected_count / 60:.0f} CPU minutes")
    return 0


if __name__ == "__main__":
    sys.exit(main())
