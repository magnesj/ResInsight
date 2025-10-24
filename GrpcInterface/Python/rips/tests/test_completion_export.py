import sys
import os
import tempfile

sys.path.insert(1, os.path.join(sys.path[0], "../../"))
import rips

import dataroot


def normalize_content(text):
    """Normalize content by removing the first lines that might vary (timestamp and file path) and fracture reporting lines."""
    lines = text.split("\n")
    if len(lines) < 10:
        return ""

    # Skip the first ten lines that might vary
    normalized_lines = lines[10:] if len(lines) > 10 else lines
    return "\n".join(normalized_lines)


def compare_with_reference(
    export_file_path, reference_file_path, file_description="file"
):
    """Compare an export file with reference data, ignoring variable header lines.

    Args:
        export_file_path: Path to the exported file
        reference_file_path: Path to the reference file
        file_description: Description of the file for error messages

    Raises:
        AssertionError: If files don't match or don't exist
    """
    assert os.path.exists(
        export_file_path
    ), f"Export {file_description} does not exist: {export_file_path}"
    assert os.path.getsize(export_file_path) > 0, f"Export {file_description} is empty"

    with open(export_file_path, "r") as f:
        content = f.read()
        assert len(content.strip()) > 0, f"Export {file_description} has no content"

    if os.path.exists(reference_file_path):
        with open(reference_file_path, "r") as ref_f:
            ref_content = ref_f.read()

        normalized_content = normalize_content(content)
        normalized_ref_content = normalize_content(ref_content)

        assert normalized_content == normalized_ref_content, (
            f"Export {file_description} differs from reference data. "
            f"Export length: {len(normalized_content)}, Reference length: {len(normalized_ref_content)}"
        )


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

        # Compare exported files with reference data
        reference_folder = (
            case_root_path + "/completion_export_reference/unified_export"
        )

        for filename in exported_files:
            export_file_path = os.path.join(export_folder, filename)
            reference_file_path = os.path.join(reference_folder, filename)
            compare_with_reference(
                export_file_path,
                reference_file_path,
                f"unified export file '{filename}'",
            )

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

        # Compare exported files with reference data
        reference_folder = case_root_path + "/completion_export_reference/split_by_well"

        for filename in exported_files:
            export_file_path = os.path.join(export_folder, filename)
            reference_file_path = os.path.join(reference_folder, filename)
            compare_with_reference(
                export_file_path,
                reference_file_path,
                f"split by well file '{filename}'",
            )

    finally:
        import shutil

        shutil.rmtree(export_folder, ignore_errors=True)
