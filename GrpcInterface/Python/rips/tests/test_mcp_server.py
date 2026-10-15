import asyncio
import math
import sys
import types
from pathlib import Path
from types import SimpleNamespace

import grpc
import pytest

from rips import RipsError, mcp_server

import dataroot


class FakeCase:
    id = 7
    name = "Example"

    def cell_count(self):
        return SimpleNamespace(active_cell_count=2, reservoir_cell_count=3)

    def grids(self):
        return [object()]

    def time_steps(self):
        return [object(), object()]

    def available_properties(self, property_type, porosity_model):
        assert property_type == "DYNAMIC_NATIVE"
        assert porosity_model == "MATRIX_MODEL"
        return ["PORO", "PERMX"]

    def grid_property_async(self, *args):
        yield SimpleNamespace(values=[1.0, 2.0])
        yield SimpleNamespace(values=[math.nan, math.inf, 3.0])

    def simulation_wells(self):
        return [SimpleNamespace(name="WELL-1")]


class FakeInstance:
    port = 50052
    project = SimpleNamespace(
        cases=lambda: [FakeCase()],
        case=lambda case_id: FakeCase() if case_id == 7 else None,
        summary_cases=lambda: [],
    )

    def version_string(self):
        return "2026.1"


def test_connect_uses_port_environment_variable(monkeypatch):
    calls = []
    monkeypatch.setenv("RESINSIGHT_PORT", "50052")
    monkeypatch.setattr(
        mcp_server.Instance,
        "find",
        staticmethod(lambda **kwargs: calls.append(kwargs) or FakeInstance()),
    )

    result = mcp_server.ResInsightMCP().connect()

    assert calls == [{"start_port": 50052, "end_port": 50053}]
    assert result == {
        "connected": True,
        "host": "localhost",
        "port": 50052,
        "version": "2026.1",
    }


def test_connect_reports_unavailable_resinsight_actionably(monkeypatch):
    def unavailable(**kwargs):
        raise RipsError("not available", code=grpc.StatusCode.UNAVAILABLE)

    monkeypatch.setattr(
        mcp_server.Instance,
        "find",
        staticmethod(unavailable),
    )

    with pytest.raises(RuntimeError, match="call connect with launch=true"):
        mcp_server.ResInsightMCP().connect(port=50052)

    with pytest.raises(ValueError, match="localhost"):
        mcp_server.ResInsightMCP().connect(host="remote.example")


def test_read_only_tools_return_summaries_and_statistics():
    service = mcp_server.ResInsightMCP(read_only=True)
    service.instance = FakeInstance()

    assert service.list_cases() == [{"id": 7, "name": "Example", "type": "FakeCase"}]
    assert service.list_grid_properties(7) == ["PORO", "PERMX"]
    assert service.list_wells(7)["wells"] == ["WELL-1"]
    assert service.case_info(7)["grid_count"] == 1

    statistics = service.get_grid_property_statistics(7, "PORO")
    assert statistics["value_count"] == 3
    assert statistics["invalid_value_count"] == 2
    assert statistics["minimum"] == 1.0
    assert statistics["maximum"] == 3.0
    assert statistics["mean"] == 2.0
    assert "values" not in statistics


def test_summary_values_are_paged_and_validated():
    summary = SimpleNamespace(
        summary_vector_values=lambda vector: SimpleNamespace(values=[1, 2, 3, 4])
    )
    project = SimpleNamespace(summary_case=lambda case_id: summary)
    service = mcp_server.ResInsightMCP()
    service.instance = SimpleNamespace(project=project)

    result = service.get_summary_values(7, "FOPT", start=1, limit=2)

    assert result["total_count"] == 4
    assert result["values"] == [2, 3]
    with pytest.raises(ValueError):
        service.get_summary_values(7, "FOPT", limit=1001)


def test_generic_pdm_tools_use_discovered_object_ids(monkeypatch):
    class FakePdmObject:
        well_name = "WELL-A"

        def __init__(self):
            self.parameters = {"WellName": "WELL-A"}

        def address(self):
            return 123

        def pb2_object(self):
            return SimpleNamespace(
                class_keyword="SimulationWell", parameters=self.parameters
            )

        def set_value(self, field_name, value):
            self.well_name = value
            self.parameters["WellName"] = value

        def rename_well(self, name):
            self.well_name = name
            return self.well_name

    generated_package = types.ModuleType("rips.generated")
    generated_package.__path__ = []
    generated_classes = types.ModuleType("rips.generated.generated_classes")
    generated_classes.class_from_keyword = lambda keyword: (
        FakePdmObject if keyword == "SimulationWell" else None
    )
    resinsight_classes = types.ModuleType("rips.resinsight_classes")
    resinsight_classes.class_dict = lambda: {"SimulationWell": FakePdmObject}
    monkeypatch.setitem(sys.modules, "rips.generated", generated_package)
    monkeypatch.setitem(
        sys.modules, "rips.generated.generated_classes", generated_classes
    )
    monkeypatch.setitem(sys.modules, "rips.resinsight_classes", resinsight_classes)

    pdm_object = FakePdmObject()
    service = mcp_server.ResInsightMCP()
    service.instance = SimpleNamespace(
        project=SimpleNamespace(descendants=lambda object_class: [pdm_object])
    )
    assert service.list_pdm_classes()["class_keywords"] == ["SimulationWell"]
    discovered = service.list_pdm_objects("SimulationWell")
    object_id = discovered["objects"][0]["object_id"]

    assert object_id == "123"
    assert service.pdm_get_fields(object_id)["fields"]["well_name"] == "WELL-A"
    assert service.pdm_set_field(object_id, "well_name", "WELL-B")["updated"]
    assert (
        service.pdm_call_method(object_id, "rename_well", {"name": "WELL-C"})
        == "WELL-C"
    )


def test_read_only_blocks_mutations():
    service = mcp_server.ResInsightMCP(read_only=True)

    with pytest.raises(PermissionError, match="read-only"):
        service.load_case("case.EGRID")
    with pytest.raises(PermissionError, match="read-only"):
        service.export_grid_property(7, "PORO", "poro.csv")
    with pytest.raises(PermissionError, match="read-only"):
        service.export_snapshots()


def test_property_export_is_confined_to_export_directory(tmp_path):
    service = mcp_server.ResInsightMCP(export_directory=str(tmp_path))
    service.instance = FakeInstance()

    exported = service.export_grid_property(7, "PORO", "poro.txt")

    assert exported == str(tmp_path / "poro.txt")
    assert (tmp_path / "poro.txt").read_text(
        encoding="utf-8"
    ) == "1.0\n2.0\nnan\ninf\n3.0\n"
    with pytest.raises(ValueError, match="simple file name"):
        service.export_grid_property(7, "PORO", "../escape.txt")


def test_snapshot_export_uses_an_isolated_export_subdirectory(tmp_path):
    def export_snapshots(**kwargs):
        (Path(kwargs["export_folder"]) / "view.png").write_text(
            "image", encoding="utf-8"
        )

    project = SimpleNamespace(export_snapshots=export_snapshots)
    service = mcp_server.ResInsightMCP(export_directory=str(tmp_path))
    service.instance = SimpleNamespace(project=project)

    result = service.export_snapshots(prefix="run_")

    assert result["file_count"] == 1
    assert Path(result["files"][0]).name == "view.png"
    with pytest.raises(ValueError, match="directory path"):
        service.export_snapshots(prefix="../escape")


def test_create_server_registers_mutations_only_when_enabled():
    read_only_tools = asyncio.run(mcp_server.create_server(read_only=True).list_tools())
    mutable_tools = asyncio.run(mcp_server.create_server().list_tools())
    read_only_names = {tool.name for tool in read_only_tools}
    mutable_names = {tool.name for tool in mutable_tools}

    assert "get_grid_property_statistics" in read_only_names
    assert "list_pdm_classes" in read_only_names
    assert "load_case" not in read_only_names
    assert "export_grid_property" not in read_only_names
    assert "export_snapshots" not in read_only_names
    assert "pdm_set_field" not in read_only_names
    assert "pdm_call_method" not in read_only_names
    assert "load_case" in mutable_names
    assert "export_grid_property" in mutable_names
    assert "export_snapshots" in mutable_names
    assert "pdm_set_field" in mutable_names
    assert "pdm_call_method" in mutable_names


def test_mcp_workflow_with_running_resinsight(rips_instance, initialize_test):
    service = mcp_server.ResInsightMCP()
    service.instance = rips_instance
    case_path = dataroot.PATH + "/TEST10K_FLT_LGR_NNC/TEST10K_FLT_LGR_NNC.EGRID"

    loaded_case = service.load_case(case_path)
    properties = service.list_grid_properties(
        loaded_case["id"], property_type="STATIC_NATIVE"
    )
    statistics = service.get_grid_property_statistics(
        loaded_case["id"],
        "PORO",
        time_step=0,
        property_type="STATIC_NATIVE",
    )

    assert loaded_case["id"] == 0
    assert "PORO" in properties
    assert statistics["value_count"] > 0
