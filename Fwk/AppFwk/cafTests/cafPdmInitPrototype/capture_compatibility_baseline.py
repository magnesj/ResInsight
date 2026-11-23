#!/usr/bin/env python3
"""Capture and compare a compatibility baseline for issue #1202.

The migration from the CAF_PDM_Init* macros to functions must not change generated Python code or
project files. This script captures both from a ResInsight executable:

  1. The generated Python classes (ResInsight --console --generate).
  2. Project files. Each project in PROJECTS is opened and saved. The saved file is opened and saved
     once more, and the two saved files must be identical.

Capture a baseline before a change, capture again after the change, and compare:

    python capture_compatibility_baseline.py capture --resinsight build/ResInsight.exe --output baseline
    python capture_compatibility_baseline.py capture --resinsight build/ResInsight.exe --output candidate
    python capture_compatibility_baseline.py compare baseline candidate

The file name of the project and date fields with the current time as default value are removed from
the saved project files. Other file paths are kept, so both captures must be made from the same
checkout folder. Requires the Python packages used by rips (grpcio and protobuf).
"""

import argparse
import datetime
import difflib
import hashlib
import json
import os
import re
import subprocess
import sys

REPO_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", "..", "..", ".."))

# Tracked project files covering wells, completions, fractures, well log plots and summary plots
PROJECTS = [
    "TestModels/TEST10K_FLT_LGR_NNC/10KWithWellLog.rsp",
    "TestModels/TEST10K_FLT_LGR_NNC/small-completion-export-fractures.rsp",
    "TestModels/TEST10K_FLT_LGR_NNC/small-completion-export-fishbones.rsp",
    "TestModels/TEST10K_FLT_LGR_NNC/well_completions_export.rsp",
    "TestModels/msw-export/project-files/multiple_laterals.rsp",
]


def sha256(path):
    with open(path, "rb") as file:
        return hashlib.sha256(file.read()).hexdigest()


def normalized_project_text(path, capture_time):
    """Remove the content that differs between two captures of the same project.

    The file name of the project is stored in the file, and some date fields have the time the
    object was created as default value.
    """
    with open(path, encoding="utf-8") as file:
        text = file.read()

    text = re.sub(r"<DocumentFileName>.*?</DocumentFileName>", "<DocumentFileName></DocumentFileName>", text)

    def mask_current_time(match):
        try:
            value = datetime.datetime.strptime(match.group(0), "%Y_%m_%d-%H:%M:%S")
        except ValueError:
            return match.group(0)
        if abs((value - capture_time).total_seconds()) < 3600:
            return "CURRENT_TIME"
        return match.group(0)

    return re.sub(r"\d{4}_\d{2}_\d{2}-\d{2}:\d{2}:\d{2}", mask_current_time, text)


def capture(args):
    output = os.path.abspath(args.output)
    os.makedirs(os.path.join(output, "projects"), exist_ok=True)
    executable = os.path.abspath(args.resinsight)

    manifest = {"files": {}, "errors": []}
    capture_time = datetime.datetime.now()

    generated = os.path.join(output, "generated_classes.py")
    subprocess.run([executable, "--console", "--generate", generated], check=True)
    manifest["files"]["generated_classes.py"] = sha256(generated)

    sys.path.insert(0, os.path.join(REPO_ROOT, "GrpcInterface", "Python"))
    import rips

    instance = rips.Instance.launch(resinsight_executable=executable, console=True)
    if not instance:
        print("Could not launch ResInsight")
        return 1

    try:
        for project_path in PROJECTS:
            name = os.path.splitext(os.path.basename(project_path))[0]
            first = os.path.join(output, "projects", name + ".rsp")
            second = os.path.join(output, "projects", name + ".second.rsp")

            instance.project.open(os.path.join(REPO_ROOT, project_path))
            instance.project.save(first)
            instance.project.close()

            instance.project.open(first)
            instance.project.save(second)
            instance.project.close()

            first_text = normalized_project_text(first, capture_time)
            second_text = normalized_project_text(second, capture_time)

            if first_text != second_text:
                manifest["errors"].append(f"{name}: project file changes when it is saved a second time")

            # The normalized file is the one to compare with another capture
            with open(first, "w", encoding="utf-8") as file:
                file.write(first_text)

            manifest["files"]["projects/" + name + ".rsp"] = sha256(first)
            os.remove(second)
            print(f"captured {name}")
    finally:
        instance.exit()

    with open(os.path.join(output, "manifest.json"), "w", encoding="utf-8") as file:
        json.dump(manifest, file, indent=2)

    for error in manifest["errors"]:
        print("ERROR " + error)
    for name, digest in manifest["files"].items():
        print(f"{digest}  {name}")
    return 1 if manifest["errors"] else 0


def compare(args):
    def load(folder):
        with open(os.path.join(folder, "manifest.json"), encoding="utf-8") as file:
            return json.load(file)

    baseline = load(args.baseline)
    candidate = load(args.candidate)

    differences = 0
    for name in sorted(set(baseline["files"]) | set(candidate["files"])):
        if baseline["files"].get(name) == candidate["files"].get(name):
            print(f"identical  {name}")
            continue

        differences += 1
        print(f"DIFFERENT  {name}")
        paths = [os.path.join(folder, name) for folder in (args.baseline, args.candidate)]
        if all(os.path.exists(path) for path in paths):
            texts = []
            for path in paths:
                with open(path, encoding="utf-8", errors="replace") as file:
                    texts.append(file.readlines())
            diff = list(difflib.unified_diff(texts[0], texts[1], "baseline/" + name, "candidate/" + name, n=1))
            sys.stdout.writelines(diff[:60])

    print(f"{differences} difference(s)")
    return 1 if differences else 0


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    commands = parser.add_subparsers(dest="command", required=True)

    capture_parser = commands.add_parser("capture")
    capture_parser.add_argument("--resinsight", required=True, help="Path to the ResInsight executable")
    capture_parser.add_argument("--output", required=True, help="Folder for the captured files")
    capture_parser.set_defaults(function=capture)

    compare_parser = commands.add_parser("compare")
    compare_parser.add_argument("baseline")
    compare_parser.add_argument("candidate")
    compare_parser.set_defaults(function=compare)

    args = parser.parse_args()
    return args.function(args)


if __name__ == "__main__":
    sys.exit(main())
