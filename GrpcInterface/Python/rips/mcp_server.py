"""Model Context Protocol server for the ResInsight Python API."""

from __future__ import annotations

import argparse
import math
import os
from pathlib import Path
from typing import Any

import grpc

from . import Instance, PropertyType, RipsError

_DEFAULT_PORT = 50051
_MAX_PAGE_SIZE = 1000


class ResInsightMCP:
    """MCP tool handlers backed by one ResInsight instance."""

    def __init__(
        self, read_only: bool = False, export_directory: str | None = None
    ) -> None:
        self.instance: Instance | None = None
        self.read_only = read_only
        export_path = export_directory or os.environ.get(
            "RESINSIGHT_EXPORT_DIR", str(Path.cwd() / "resinsight_exports")
        )
        self.export_directory = Path(export_path).expanduser().resolve()

    def connect(
        self,
        host: str = "localhost",
        port: int | None = None,
        launch: bool = False,
        console: bool = True,
        exe_path: str | None = None,
    ) -> dict[str, Any]:
        """Attach to a running instance, or launch one when requested."""
        if host not in ("localhost", "127.0.0.1"):
            raise ValueError("The rips API currently supports localhost connections only.")
        port = port or int(os.environ.get("RESINSIGHT_PORT", _DEFAULT_PORT))
        try:
            if launch:
                self.instance = Instance.launch(
                    resinsight_executable=exe_path
                    or os.environ.get("RESINSIGHT_EXECUTABLE", ""),
                    console=console,
                    launch_port=port,
                )
            else:
                self.instance = Instance.find(
                    start_port=port, end_port=port + 1
                )
        except (grpc.RpcError, RipsError) as exc:
            self.instance = None
            raise RuntimeError(
                f"Unable to connect to ResInsight on {host}:{port}. "
                "Start ResInsight or call connect with launch=true."
            ) from exc
        return {
            "connected": True,
            "host": host,
            "port": self.instance.port,
            "version": self.instance.version_string(),
        }

    def get_version(self) -> str:
        return self._require_instance().version_string()

    def list_cases(self) -> list[dict[str, Any]]:
        project = self._require_instance().project
        return [
            {
                "id": case.id,
                "name": str(getattr(case, "name", "")),
                "type": type(case).__name__,
            }
            for case in project.cases()
        ]

    def case_info(self, case_id: int) -> dict[str, Any]:
        case = self._get_case(case_id)
        cell_count = case.cell_count()
        return {
            "id": case.id,
            "name": str(getattr(case, "name", "")),
            "type": type(case).__name__,
            "active_cell_count": int(cell_count.active_cell_count),
            "reservoir_cell_count": int(cell_count.reservoir_cell_count),
            "grid_count": len(case.grids()),
            "time_step_count": len(case.time_steps()),
        }

    def list_grid_properties(
        self,
        case_id: int,
        property_type: str = "DYNAMIC_NATIVE",
        porosity_model: str = "MATRIX_MODEL",
    ) -> list[str]:
        case = self._get_case(case_id)
        return list(case.available_properties(PropertyType(property_type), porosity_model))

    def get_grid_property_statistics(
        self,
        case_id: int,
        property_name: str,
        time_step: int = 0,
        property_type: str = "DYNAMIC_NATIVE",
        grid_index: int = 0,
        porosity_model: str = "MATRIX_MODEL",
    ) -> dict[str, Any]:
        """Compute bounded-memory statistics without returning cell arrays."""
        case = self._get_case(case_id)
        count = 0
        invalid_count = 0
        minimum = math.inf
        maximum = -math.inf
        mean = 0.0
        total = 0.0
        for chunk in case.grid_property_async(
            PropertyType(property_type),
            property_name,
            time_step,
            grid_index,
            porosity_model,
        ):
            for raw_value in chunk.values:
                value = float(raw_value)
                if not math.isfinite(value):
                    invalid_count += 1
                    continue
                count += 1
                minimum = min(minimum, value)
                maximum = max(maximum, value)
                total += value
                mean += (value - mean) / count
        return {
            "case_id": case_id,
            "property_name": property_name,
            "property_type": property_type,
            "time_step": time_step,
            "grid_index": grid_index,
            "value_count": count,
            "invalid_value_count": invalid_count,
            "minimum": minimum if count else None,
            "maximum": maximum if count else None,
            "mean": mean if count else None,
            "sum": total if count else None,
        }

    def list_wells(self, case_id: int) -> list[str]:
        case = self._get_case(case_id)
        return [
            str(getattr(well, "name", ""))
            for well in case.simulation_wells()[:_MAX_PAGE_SIZE]
        ]

    def list_summary_cases(self) -> list[dict[str, Any]]:
        return [
            {
                "id": summary_case.id,
                "name": str(getattr(summary_case, "name", "")),
                "type": type(summary_case).__name__,
            }
            for summary_case in self._require_instance().project.summary_cases()
        ]

    def list_summary_vectors(self, case_id: int) -> list[str]:
        summary_case = self._get_summary_case(case_id)
        return list(summary_case.available_addresses().values)

    def get_summary_values(
        self, case_id: int, vector: str, start: int = 0, limit: int = 100
    ) -> dict[str, Any]:
        if start < 0 or limit < 1 or limit > _MAX_PAGE_SIZE:
            raise ValueError(f"start must be >= 0 and limit must be between 1 and {_MAX_PAGE_SIZE}.")
        values = self._get_summary_case(case_id).summary_vector_values(vector).values
        return {
            "case_id": case_id,
            "vector": vector,
            "total_count": len(values),
            "start": start,
            "values": list(values[start : start + limit]),
        }

    def load_case(self, path: str) -> dict[str, Any]:
        self._require_mutable()
        case = self._require_instance().project.load_case(path=path)
        return {"id": case.id, "name": str(getattr(case, "name", ""))}

    def import_summary_case(self, path: str) -> dict[str, Any]:
        self._require_mutable()
        summary_case = self._require_instance().project.import_summary_case(path)
        return {"id": summary_case.id, "name": str(getattr(summary_case, "name", ""))}

    def export_grid_property(
        self,
        case_id: int,
        property_name: str,
        filename: str,
        time_step: int = 0,
        property_type: str = "DYNAMIC_NATIVE",
        grid_index: int = 0,
        porosity_model: str = "MATRIX_MODEL",
    ) -> str:
        self._require_mutable()
        if not filename or Path(filename).name != filename:
            raise ValueError("filename must be a simple file name inside the configured export directory.")
        self.export_directory.mkdir(parents=True, exist_ok=True)
        output_path = (self.export_directory / filename).resolve()
        if output_path.parent != self.export_directory:
            raise ValueError("The export path must be inside the configured export directory.")
        case = self._get_case(case_id)
        with output_path.open("w", encoding="utf-8") as output_file:
            for chunk in case.grid_property_async(
                PropertyType(property_type),
                property_name,
                time_step,
                grid_index,
                porosity_model,
            ):
                for value in chunk.values:
                    output_file.write(f"{value}\n")
        return str(output_path)

    def project_resource(self) -> str:
        import json

        return json.dumps({"cases": self.list_cases(), "summary_cases": self.list_summary_cases()})

    def case_resource(self, case_id: int) -> str:
        import json

        return json.dumps(self.case_info(case_id))

    def _get_case(self, case_id: int):
        case = self._require_instance().project.case(case_id)
        if case is None:
            raise ValueError(f"No grid case with id {case_id} exists in the project.")
        return case

    def _get_summary_case(self, case_id: int):
        summary_case = self._require_instance().project.summary_case(case_id)
        if summary_case is None:
            raise ValueError(f"No summary case with id {case_id} exists in the project.")
        return summary_case

    def _require_instance(self) -> Instance:
        if self.instance is None:
            raise RuntimeError("ResInsight is not connected; call connect first.")
        return self.instance

    def _require_mutable(self) -> None:
        if self.read_only:
            raise PermissionError("This MCP server is in read-only mode.")


def create_server(read_only: bool = False, export_directory: str | None = None):
    """Create a FastMCP application with the RIPS tools and resources."""
    try:
        from mcp.server.fastmcp import FastMCP
    except ImportError as exc:
        raise RuntimeError(
            "Install the optional dependency with `pip install 'rips[mcp]'`."
        ) from exc

    service = ResInsightMCP(read_only, export_directory)
    server = FastMCP("ResInsight")

    server.tool()(service.connect)
    server.tool()(service.get_version)
    server.tool()(service.list_cases)
    server.tool()(service.case_info)
    server.tool()(service.list_grid_properties)
    server.tool()(service.get_grid_property_statistics)
    server.tool()(service.list_wells)
    server.tool()(service.list_summary_cases)
    server.tool()(service.list_summary_vectors)
    server.tool()(service.get_summary_values)
    server.tool()(service.load_case)
    server.tool()(service.import_summary_case)
    server.tool()(service.export_grid_property)
    server.resource("resinsight://project")(service.project_resource)
    server.resource("resinsight://case/{case_id}")(service.case_resource)

    @server.prompt()
    def summarize_ensemble() -> str:
        """Guide an agent to summarize the currently loaded ensemble."""
        return (
            "List the grid cases and summary cases. Compare available summary "
            "vectors and report key differences without returning raw grid arrays."
        )

    return server


def main() -> None:
    parser = argparse.ArgumentParser(description="ResInsight MCP server")
    parser.add_argument("--read-only", action="store_true")
    parser.add_argument("--export-directory")
    arguments = parser.parse_args()
    create_server(arguments.read_only, arguments.export_directory).run(
        transport="stdio"
    )


if __name__ == "__main__":
    main()
