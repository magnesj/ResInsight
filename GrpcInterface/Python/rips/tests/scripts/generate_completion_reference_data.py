#!/usr/bin/env python3
"""
Script to generate reference data for completion export tests.
This script runs the actual completion exports and stores the results as reference data.
"""

import sys
import os
import tempfile
import shutil

sys.path.insert(1, os.path.join(sys.path[0], "../../"))
import rips

# Define path to test models (same as dataroot.py)
TEST_MODELS_PATH = "../../../../TestModels"


def generate_unified_export_reference(rips_instance):
    """Generate reference data for unified export test."""
    print("Generating unified export reference data...")

    case_root_path = TEST_MODELS_PATH + "/TEST10K_FLT_LGR_NNC"
    project_path = case_root_path + "/well_completions_export.rsp"
    reference_folder = case_root_path + "/completion_export_reference/unified_export"

    project = rips_instance.project.open(path=project_path)

    export_folder = tempfile.mkdtemp()

    try:
        rips_instance.set_export_folder(export_type="COMPLETIONS", path=export_folder)

        case = project.cases()[0]

        # Check available wells
        well_paths = rips_instance.project.well_path_collection().well_paths()
        available_wells = [wp.name for wp in well_paths]

        if not available_wells:
            print("No well paths available for export")
            return False

        well_names_to_use = (
            ["Well-1"] if "Well-1" in available_wells else available_wells[:1]
        )

        print(f"Exporting for wells: {well_names_to_use}")

        case.export_well_path_completions(
            time_step=0,
            well_path_names=well_names_to_use,
            file_split="UNIFIED_FILE",
            include_perforations=True,
            include_fishbones=True,
            export_welspec=True,
            export_comments=True,
        )

        exported_files = [
            f
            for f in os.listdir(export_folder)
            if os.path.isfile(os.path.join(export_folder, f))
        ]

        if exported_files:
            # Copy all exported files to reference folder
            for filename in exported_files:
                src_path = os.path.join(export_folder, filename)
                dst_path = os.path.join(reference_folder, filename)
                shutil.copy2(src_path, dst_path)
                print(f"Reference data saved to: {dst_path}")
            return True
        else:
            print("No export files were created")
            return False

    finally:
        shutil.rmtree(export_folder, ignore_errors=True)


def generate_split_by_well_reference(rips_instance):
    """Generate reference data for split by well export test."""
    print("Generating split by well export reference data...")

    case_root_path = TEST_MODELS_PATH + "/TEST10K_FLT_LGR_NNC"
    project_path = case_root_path + "/well_completions_export.rsp"
    reference_folder = case_root_path + "/completion_export_reference/split_by_well"

    project = rips_instance.project.open(path=project_path)

    export_folder = tempfile.mkdtemp()

    try:
        rips_instance.set_export_folder(export_type="COMPLETIONS", path=export_folder)

        case = project.cases()[0]

        # Check available wells
        well_paths = rips_instance.project.well_path_collection().well_paths()
        available_wells = [wp.name for wp in well_paths]

        if not available_wells:
            print("No well paths available for export")
            return False

        # Use available wells, or default wells if they exist
        test_wells = []
        for well_name in ["Well-1", "Well-2"]:
            if well_name in available_wells:
                test_wells.append(well_name)

        # If neither Well-1 nor Well-2 exist, use the first two available wells
        if not test_wells:
            test_wells = available_wells[:2]

        print(f"Exporting for wells: {test_wells}")

        case.export_well_path_completions(
            time_step=0,
            well_path_names=test_wells,
            file_split="SPLIT_ON_WELL",
            include_perforations=True,
            include_fishbones=False,
        )

        exported_files = [
            f
            for f in os.listdir(export_folder)
            if os.path.isfile(os.path.join(export_folder, f))
        ]

        if exported_files:
            # Copy all exported files to reference folder
            for filename in exported_files:
                src_path = os.path.join(export_folder, filename)
                dst_path = os.path.join(reference_folder, filename)
                shutil.copy2(src_path, dst_path)
                print(f"Reference data saved to: {dst_path}")
            return True
        else:
            print("No export files were created")
            return False

    finally:
        shutil.rmtree(export_folder, ignore_errors=True)


def main():
    """Main function to generate all reference data."""
    print("Starting reference data generation...")

    try:
        # Connect to ResInsight
        rips_instance = rips.Instance.find()
        if rips_instance is None:
            print("Could not connect to ResInsight. Make sure ResInsight is running.")
            return False

        print("Connected to ResInsight")

        # Generate reference data for each test
        success_count = 0

        if generate_unified_export_reference(rips_instance):
            success_count += 1

        if generate_split_by_well_reference(rips_instance):
            success_count += 1

        print(
            f"\nReference data generation completed. {success_count}/3 tests generated reference data."
        )
        return success_count > 0

    except Exception as e:
        print(f"Error generating reference data: {e}")
        return False


if __name__ == "__main__":
    success = main()
    sys.exit(0 if success else 1)
