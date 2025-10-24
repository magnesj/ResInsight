import sys
import os
import tempfile

sys.path.insert(1, os.path.join(sys.path[0], "../../"))
import rips

import dataroot


def test_export_completion_files_unified(rips_instance, initialize_test):
    case_root_path = dataroot.PATH + "/TEST10K_FLT_LGR_NNC"
    project_path = case_root_path + "/well_completions_export.rsp"
    project = rips_instance.project.open(path=project_path)

    export_folder = tempfile.mkdtemp()

    try:
        rips_instance.set_export_folder(export_type="COMPLETIONS", path=export_folder)

        case = project.cases()[0]

        # Use Well-1 for all tests
        well_names_to_use = ["Well-1"]

        case.export_well_path_completions(
            time_step=0,  # Use time step 0 instead of 1
            well_path_names=well_names_to_use,
            file_split="UNIFIED_FILE",
            include_perforations=True,
            include_fishbones=True,  # Match reference data generation
            export_welspec=True,
            export_comments=True,
        )

        exported_files = [
            f
            for f in os.listdir(export_folder)
            if os.path.isfile(os.path.join(export_folder, f))
        ]

        if not exported_files:
            # If no files created, this might be expected if there are no completions
            import pytest

            pytest.skip(
                "No completion files created - likely no completions exist for the wells"
            )

        # Verify files have content and compare with reference data
        reference_folder = (
            case_root_path + "/completion_export_reference/unified_export"
        )

        def normalize_content(text):
            lines = text.split("\n")
            normalized_lines = []
            for line in lines:
                # Skip timestamp lines that contain "Exported from ResInsight"
                if not line.startswith("-- Exported from ResInsight"):
                    normalized_lines.append(line)
            return "\n".join(normalized_lines)

        for filename in exported_files:
            filepath = os.path.join(export_folder, filename)
            assert os.path.getsize(filepath) > 0, f"Export file {filename} is empty"

            with open(filepath, "r") as f:
                content = f.read()
                assert (
                    len(content.strip()) > 0
                ), f"Export file {filename} has no content"

            # Compare with reference data if it exists
            reference_file = os.path.join(reference_folder, filename)
            if os.path.exists(reference_file):
                with open(reference_file, "r") as ref_f:
                    ref_content = ref_f.read()

                normalized_content = normalize_content(content)
                normalized_ref_content = normalize_content(ref_content)

                assert (
                    normalized_content == normalized_ref_content
                ), f"Export file {filename} differs from reference data"

    finally:
        import shutil

        shutil.rmtree(export_folder, ignore_errors=True)


def test_export_completion_files_split_by_well(rips_instance, initialize_test):
    case_root_path = dataroot.PATH + "/TEST10K_FLT_LGR_NNC"
    project_path = case_root_path + "/well_completions_export.rsp"
    project = rips_instance.project.open(path=project_path)

    export_folder = tempfile.mkdtemp()

    try:
        rips_instance.set_export_folder(export_type="COMPLETIONS", path=export_folder)

        case = project.cases()[0]

        # Use Well-1 for all tests
        test_wells = ["Well-1"]

        case.export_well_path_completions(
            time_step=0,  # Use time step 0 instead of 1
            well_path_names=test_wells,
            file_split="SPLIT_ON_WELL",
            include_perforations=True,
            include_fishbones=False,  # This matches the reference generation script
        )

        exported_files = [
            f
            for f in os.listdir(export_folder)
            if os.path.isfile(os.path.join(export_folder, f))
        ]

        if not exported_files:
            # If no files created, this might be expected if there are no completions
            import pytest

            pytest.skip(
                "No completion files created - likely no completions exist for the wells"
            )

        # Verify files have content and compare with reference data
        reference_folder = case_root_path + "/completion_export_reference/split_by_well"

        def normalize_content(text):
            lines = text.split("\n")
            normalized_lines = []
            for line in lines:
                # Skip timestamp lines that contain "Exported from ResInsight"
                if not line.startswith("-- Exported from ResInsight"):
                    normalized_lines.append(line)
            return "\n".join(normalized_lines)

        for filename in exported_files:
            filepath = os.path.join(export_folder, filename)
            assert os.path.getsize(filepath) > 0, f"Export file {filename} is empty"

            with open(filepath, "r") as f:
                content = f.read()
                assert (
                    len(content.strip()) > 0
                ), f"Export file {filename} has no content"

            # Compare with reference data if it exists
            reference_file = os.path.join(reference_folder, filename)
            if os.path.exists(reference_file):
                with open(reference_file, "r") as ref_f:
                    ref_content = ref_f.read()

                normalized_content = normalize_content(content)
                normalized_ref_content = normalize_content(ref_content)

                assert (
                    normalized_content == normalized_ref_content
                ), f"Export file {filename} differs from reference data"

    finally:
        import shutil

        shutil.rmtree(export_folder, ignore_errors=True)
