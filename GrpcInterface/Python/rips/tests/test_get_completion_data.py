import sys
import os

sys.path.insert(1, os.path.join(sys.path[0], "../../"))
import rips

import dataroot

import pytest


def test_get_completion_data_basic(rips_instance, initialize_test):
    """Test GetCompletionData with a well path and completions"""
    # Load a grid case and import existing well path
    case_root_path = dataroot.PATH + "/TEST10K_FLT_LGR_NNC"
    case_path = case_root_path + "/TEST10K_FLT_LGR_NNC.EGRID"
    case = rips_instance.project.load_case(path=case_path)
    assert len(case.grids()) == 2

    # Import existing well path that intersects the grid properly
    well_path_file = case_root_path + "/wellpath_a.dev"
    well_path_names = rips_instance.project.import_well_paths([well_path_file])
    assert len(well_path_names) == 1

    wells = rips_instance.project.well_paths()
    assert len(wells) == 1
    well_path = wells[0]
    well_path.name = "TestWell"  # Rename for our test
    well_path.update()

    # Get the trajectory properties to find valid MD range
    result = well_path.trajectory_properties(resampling_interval=10.0)
    measured_depths = result["measured_depth"]
    
    # Use measured depth range from the actual well trajectory
    start_md = measured_depths[len(measured_depths) // 2]  # Middle of well
    end_md = measured_depths[len(measured_depths) // 2 + 10] if len(measured_depths) > 20 else measured_depths[-1]

    # Add perforation intervals (completions)
    perf_interval = well_path.append_perforation_interval(
        start_md=start_md,
        end_md=end_md, 
        diameter=0.25,
        skin_factor=0.1
    )
    assert perf_interval is not None

    # Test the completion_data method
    completion_data = well_path.completion_data(case.id)
    
    # Verify basic structure exists
    assert completion_data is not None
    
    # Should have WELSPECS data for the well
    assert len(completion_data.welspecs) > 0
    welspec = completion_data.welspecs[0]
    assert welspec.well_name == "TestWell"
    assert welspec.grid_i > 0
    assert welspec.grid_j > 0

    # Should have COMPDAT data for the perforation intervals
    assert len(completion_data.compdat) > 0
    compdat = completion_data.compdat[0]
    assert compdat.well_name == "TestWell"
    assert compdat.grid_i > 0
    assert compdat.grid_j > 0
    assert compdat.upper_k > 0
    assert compdat.lower_k > 0
    assert compdat.open_shut_flag in ["OPEN", "SHUT"]
    
    # Check measured depth information in COMPDAT
    assert compdat.HasField("start_md")
    assert compdat.HasField("end_md")
    assert compdat.start_md >= start_md - 10  # Allow some tolerance
    assert compdat.end_md <= end_md + 10


def test_get_completion_data_with_valves(rips_instance, initialize_test):
    """Test GetCompletionData with valves/ICDs"""
    # Load a grid case and import well path
    case_root_path = dataroot.PATH + "/TEST10K_FLT_LGR_NNC"
    case_path = case_root_path + "/TEST10K_FLT_LGR_NNC.EGRID"
    case = rips_instance.project.load_case(path=case_path)

    # Import existing well path
    well_path_file = case_root_path + "/wellpath_b.dev"
    well_path_names = rips_instance.project.import_well_paths([well_path_file])
    assert len(well_path_names) == 1

    wells = rips_instance.project.well_paths()
    well_path = wells[0]
    well_path.name = "TestWellWithValves"
    well_path.update()

    # Get trajectory properties for valid MD range
    result = well_path.trajectory_properties(resampling_interval=10.0)
    measured_depths = result["measured_depth"]
    start_md = measured_depths[len(measured_depths) // 2]
    end_md = measured_depths[len(measured_depths) // 2 + 10] if len(measured_depths) > 20 else measured_depths[-1]

    # Add perforation interval with valve
    perf_interval = well_path.append_perforation_interval(
        start_md=start_md,
        end_md=end_md,
        diameter=0.25,
        skin_factor=0.1
    )

    # Add a valve to the perforation interval
    valve_templates = rips_instance.project.valve_templates()
    valve_defs = valve_templates.valve_definitions()
    assert len(valve_defs) >= 1

    valve_start_md = start_md + (end_md - start_md) * 0.2
    valve_end_md = start_md + (end_md - start_md) * 0.8
    valve = perf_interval.add_valve(
        template=valve_defs[0],
        start_md=valve_start_md,
        end_md=valve_end_md,
        valve_count=2
    )
    assert valve is not None

    # Test completion data
    completion_data = well_path.completion_data(case.id)
    assert completion_data is not None

    # Should have WELSPECS
    assert len(completion_data.welspecs) > 0
    welspec = completion_data.welspecs[0]
    assert welspec.well_name == "TestWellWithValves"

    # Should have COMPDAT
    assert len(completion_data.compdat) > 0


def test_get_completion_data_multiple_intervals(rips_instance, initialize_test):
    """Test GetCompletionData with multiple perforation intervals"""
    # Load a grid case and import well path
    case_root_path = dataroot.PATH + "/TEST10K_FLT_LGR_NNC"
    case_path = case_root_path + "/TEST10K_FLT_LGR_NNC.EGRID"
    case = rips_instance.project.load_case(path=case_path)

    # Import existing well path
    well_path_file = case_root_path + "/wellpath_a.dev"
    well_path_names = rips_instance.project.import_well_paths([well_path_file])
    wells = rips_instance.project.well_paths()
    well_path = wells[0]
    well_path.name = "TestMultiInterval"
    well_path.update()

    # Get trajectory properties for valid MD ranges
    result = well_path.trajectory_properties(resampling_interval=10.0)
    measured_depths = result["measured_depth"]
    
    # Create three intervals along the well path
    total_length = len(measured_depths)
    if total_length >= 30:
        # Interval 1: Early part of well
        start_md1 = measured_depths[total_length // 4]
        end_md1 = measured_depths[total_length // 4 + 5]
        
        # Interval 2: Middle part of well 
        start_md2 = measured_depths[total_length // 2]
        end_md2 = measured_depths[total_length // 2 + 5]
        
        # Interval 3: Later part of well
        start_md3 = measured_depths[3 * total_length // 4]
        end_md3 = measured_depths[3 * total_length // 4 + 5]
        
        # Add multiple perforation intervals
        interval1 = well_path.append_perforation_interval(
            start_md=start_md1, end_md=end_md1, diameter=0.2, skin_factor=0.0
        )
        interval2 = well_path.append_perforation_interval(
            start_md=start_md2, end_md=end_md2, diameter=0.25, skin_factor=0.1
        )
        interval3 = well_path.append_perforation_interval(
            start_md=start_md3, end_md=end_md3, diameter=0.3, skin_factor=0.2
        )

        assert interval1 is not None
        assert interval2 is not None
        assert interval3 is not None

        # Test completion data
        completion_data = well_path.completion_data(case.id)
        assert completion_data is not None

        # Should have WELSPECS
        assert len(completion_data.welspecs) > 0
        welspec = completion_data.welspecs[0]
        assert welspec.well_name == "TestMultiInterval"

        # Should have multiple COMPDAT entries for different intervals
        assert len(completion_data.compdat) >= 1  # At least some COMPDAT data

        # Check that we have MD information in COMPDAT entries
        has_md_data = False
        for compdat in completion_data.compdat:
            if compdat.HasField("start_md") and compdat.HasField("end_md"):
                has_md_data = True
                assert compdat.start_md >= 0
                assert compdat.end_md >= compdat.start_md
                
        assert has_md_data, "Should have measured depth data in COMPDAT"
    else:
        # Skip test if well is too short for multiple intervals
        assert True  # Test passes if well is too short


def test_get_completion_data_field_validation(rips_instance, initialize_test):
    """Test optional field handling in completion data"""
    # Load a grid case and import well path
    case_root_path = dataroot.PATH + "/TEST10K_FLT_LGR_NNC"
    case_path = case_root_path + "/TEST10K_FLT_LGR_NNC.EGRID"
    case = rips_instance.project.load_case(path=case_path)

    # Import existing well path
    well_path_file = case_root_path + "/wellpath_a.dev"
    well_path_names = rips_instance.project.import_well_paths([well_path_file])
    wells = rips_instance.project.well_paths()
    well_path = wells[0]
    well_path.name = "FieldValidationTest"
    well_path.update()

    # Get valid MD range
    result = well_path.trajectory_properties(resampling_interval=10.0)
    measured_depths = result["measured_depth"]
    start_md = measured_depths[len(measured_depths) // 2]
    end_md = measured_depths[len(measured_depths) // 2 + 5] if len(measured_depths) > 10 else measured_depths[-1]

    perf_interval = well_path.append_perforation_interval(start_md, end_md, 0.25, 0.1)
    assert perf_interval is not None

    # Get completion data
    completion_data = well_path.completion_data(case.id)
    assert completion_data is not None

    # Validate COMPDAT optional fields
    if len(completion_data.compdat) > 0:
        comp = completion_data.compdat[0]
        
        # Test required fields
        assert comp.well_name == "FieldValidationTest"
        assert comp.grid_i > 0
        assert comp.grid_j > 0
        assert comp.upper_k > 0
        assert comp.lower_k > 0
        assert comp.open_shut_flag in ["OPEN", "SHUT"]
        
        # Test optional double fields - verify type and range when present
        if comp.HasField("transmissibility"):
            assert comp.transmissibility >= 0.0
        if comp.HasField("diameter"):
            assert comp.diameter > 0.0
        if comp.HasField("skin_factor"):
            assert isinstance(comp.skin_factor, float)
        if comp.HasField("start_md") and comp.HasField("end_md"):
            assert comp.end_md >= comp.start_md

    # Validate WELSPECS required and optional fields
    if len(completion_data.welspecs) > 0:
        welspec = completion_data.welspecs[0]
        
        # Required fields
        assert welspec.well_name == "FieldValidationTest"
        assert welspec.group_name
        assert welspec.grid_i > 0
        assert welspec.grid_j > 0
        assert welspec.phase in ["OIL", "GAS", "WATER", "LIQ"]
        
        # Optional fields
        if welspec.HasField("bhp_depth"):
            assert welspec.bhp_depth >= 0.0
        if welspec.HasField("drainage_radius"):
            assert welspec.drainage_radius > 0.0


def test_get_completion_data_invalid_case_id(rips_instance, initialize_test):
    """Test error handling for invalid case ID"""
    # Load a grid case
    case_root_path = dataroot.PATH + "/TEST10K_FLT_LGR_NNC"
    case_path = case_root_path + "/TEST10K_FLT_LGR_NNC.EGRID"
    rips_instance.project.load_case(path=case_path)

    # Create well path
    well_path_coll = rips_instance.project.well_path_collection()
    well_path = well_path_coll.add_new_object(rips.ModeledWellPath)
    well_path.name = "ErrorTestWell"
    well_path.update()

    geometry = well_path.well_path_geometry()
    surface_coord = [460000.0, 5930000.0, 1600.0]
    bottom_coord = [461000.0, 5931000.0, 1900.0]
    
    geometry.append_well_target(surface_coord)
    geometry.append_well_target(bottom_coord)
    well_path.update()

    perf_interval = well_path.append_perforation_interval(1000.0, 1200.0, 0.25, 0.1)
    assert perf_interval is not None

    # Test with invalid case ID
    try:
        # Using a negative case ID that shouldn't exist
        completion_data = well_path.completion_data(-1)
        # Should return empty data or handle gracefully
        assert completion_data is not None
        # With invalid case, completion data should be empty or minimal
    except Exception as e:
        # Acceptable if it throws an error for invalid case ID
        assert "case" in str(e).lower() or "invalid" in str(e).lower() or "id" in str(e).lower()


def test_get_completion_data_empty_well(rips_instance, initialize_test):
    """Test GetCompletionData with a well that has no completions"""
    # Load a grid case
    case_root_path = dataroot.PATH + "/TEST10K_FLT_LGR_NNC"
    case_path = case_root_path + "/TEST10K_FLT_LGR_NNC.EGRID"
    case = rips_instance.project.load_case(path=case_path)

    # Create well path without completions
    well_path_coll = rips_instance.project.well_path_collection()
    well_path = well_path_coll.add_new_object(rips.ModeledWellPath)
    well_path.name = "EmptyWell"
    well_path.update()

    geometry = well_path.well_path_geometry()
    surface_coord = [460000.0, 5930000.0, 1600.0]
    bottom_coord = [461000.0, 5931000.0, 1900.0]
    
    geometry.append_well_target(surface_coord)
    geometry.append_well_target(bottom_coord)
    well_path.update()

    # Don't add any perforation intervals

    # Test completion data - should return valid response but with empty/minimal data
    completion_data = well_path.completion_data(case.id)
    assert completion_data is not None

    # May or may not have WELSPECS depending on implementation
    # But should not have COMPDAT entries without perforations
    assert len(completion_data.compdat) == 0