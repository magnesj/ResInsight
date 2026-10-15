"""Model Context Protocol server for the ResInsight Python API."""

from __future__ import annotations

import argparse
import enum
import math
import os
import re
import shutil
import tempfile
from functools import wraps
from pathlib import Path
from typing import Any, Callable, TypeVar

import grpc

from . import Instance, RipsError
from .pdmobject import camel_to_snake, snake_to_camel

_DEFAULT_PORT = 50051
_MAX_PAGE_SIZE = 1000

F = TypeVar("F", bound=Callable[..., Any])


def _translate_errors(function: F) -> F:
    @wraps(function)
    def wrapper(*args: Any, **kwargs: Any) -> Any:
        try:
            return function(*args, **kwargs)
        except grpc.RpcError as exc:
            if exc.code() == grpc.StatusCode.UNAVAILABLE:
                message = "ResInsight is not responding; call connect or launch it."
            else:
                message = f"ResInsight gRPC request failed: {exc.details() or exc}"
            raise RuntimeError(message) from exc
        except RipsError as exc:
            if exc.code == grpc.StatusCode.UNAVAILABLE:
                message = "ResInsight is not responding; call connect or launch it."
            else:
                message = f"ResInsight request failed: {exc}"
            raise RuntimeError(message) from exc

    return wrapper  # type: ignore[return-value]


class ResInsightMCP:
    """MCP tool handlers backed by one ResInsight instance."""

    def __init__(
        self, read_only: bool = False, export_directory: str | None = None
    ) -> None:
        self.instance: Instance | None = None
        self.read_only = read_only
        self._pdm_objects: dict[str, Any] = {}
        export_path = export_directory or os.environ.get(
            "RESINSIGHT_EXPORT_DIR", str(Path.cwd() / "resinsight_exports")
        )
        self.export_directory = Path(export_path).expanduser().resolve()

    @_translate_errors
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
            raise ValueError(
                "The rips API currently supports localhost connections only."
            )
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
                self.instance = Instance.find(start_port=port, end_port=port + 1)
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

    @_translate_errors
    def get_version(self) -> str:
        return self._require_instance().version_string()

    @_translate_errors
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

    @_translate_errors
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

    @_translate_errors
    def list_grid_properties(
        self,
        case_id: int,
        property_type: str = "DYNAMIC_NATIVE",
        porosity_model: str = "MATRIX_MODEL",
    ) -> list[str]:
        case = self._get_case(case_id)
        return list(case.available_properties(property_type, porosity_model))

    @_translate_errors
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
            property_type,
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

    @_translate_errors
    def list_wells(
        self, case_id: int, start: int = 0, limit: int = 100
    ) -> dict[str, Any]:
        if start < 0 or limit < 1 or limit > _MAX_PAGE_SIZE:
            raise ValueError(
                f"start must be >= 0 and limit must be between 1 and {_MAX_PAGE_SIZE}."
            )
        case = self._get_case(case_id)
        wells = [str(getattr(well, "name", "")) for well in case.simulation_wells()]
        return {
            "case_id": case_id,
            "total_count": len(wells),
            "start": start,
            "wells": wells[start : start + limit],
        }

    @_translate_errors
    def list_summary_cases(self) -> list[dict[str, Any]]:
        return [
            {
                "id": summary_case.id,
                "name": str(getattr(summary_case, "name", "")),
                "type": type(summary_case).__name__,
            }
            for summary_case in self._require_instance().project.summary_cases()
        ]

    @_translate_errors
    def list_summary_vectors(self, case_id: int) -> list[str]:
        summary_case = self._get_summary_case(case_id)
        return list(summary_case.available_addresses().values)

    @_translate_errors
    def list_pdm_objects(
        self, class_keyword: str, start: int = 0, limit: int = 100
    ) -> dict[str, Any]:
        if start < 0 or limit < 1 or limit > _MAX_PAGE_SIZE:
            raise ValueError(
                f"start must be >= 0 and limit must be between 1 and {_MAX_PAGE_SIZE}."
            )
        from .generated.generated_classes import class_from_keyword

        object_class = class_from_keyword(class_keyword)
        if object_class is None:
            raise ValueError(f"Unknown PDM class keyword: {class_keyword}")
        objects = self._require_instance().project.descendants(object_class)
        page = objects[start : start + limit]
        return {
            "total_count": len(objects),
            "start": start,
            "objects": [
                {
                    "object_id": self._remember_pdm_object(pdm_object),
                    "class_keyword": pdm_object.pb2_object().class_keyword,
                }
                for pdm_object in page
            ],
        }

    @_translate_errors
    def pdm_get_fields(self, object_id: str) -> dict[str, Any]:
        pdm_object = self._get_pdm_object(object_id)
        parameters = pdm_object.pb2_object().parameters
        fields = {
            camel_to_snake(keyword): self._serialize_value(
                getattr(pdm_object, camel_to_snake(keyword), value)
            )
            for keyword, value in parameters.items()
        }
        return {
            "object_id": object_id,
            "class_keyword": pdm_object.pb2_object().class_keyword,
            "fields": fields,
        }

    @_translate_errors
    def pdm_set_field(
        self, object_id: str, field_name: str, value: Any
    ) -> dict[str, Any]:
        self._require_mutable()
        if not re.fullmatch(r"[a-zA-Z][a-zA-Z0-9_]*", field_name):
            raise ValueError("field_name must be a generated PDM field name.")
        pdm_object = self._get_pdm_object(object_id)
        if snake_to_camel(field_name) not in pdm_object.pb2_object().parameters:
            raise ValueError(f"Unknown PDM field: {field_name}")
        pdm_object.set_value(field_name, value)
        return {"object_id": object_id, "field_name": field_name, "updated": True}

    @_translate_errors
    def pdm_call_method(
        self, object_id: str, method_name: str, arguments: dict[str, Any] | None = None
    ) -> Any:
        self._require_mutable()
        if method_name.startswith("_"):
            raise ValueError("Only public generated PDM methods can be called.")
        pdm_object = self._get_pdm_object(object_id)
        method = getattr(pdm_object, method_name, None)
        if not callable(method):
            raise ValueError(f"No generated PDM method named {method_name!r} exists.")
        return self._serialize_value(method(**(arguments or {})))

    @_translate_errors
    def get_summary_values(
        self, case_id: int, vector: str, start: int = 0, limit: int = 100
    ) -> dict[str, Any]:
        if start < 0 or limit < 1 or limit > _MAX_PAGE_SIZE:
            raise ValueError(
                f"start must be >= 0 and limit must be between 1 and {_MAX_PAGE_SIZE}."
            )
        values = self._get_summary_case(case_id).summary_vector_values(vector).values
        return {
            "case_id": case_id,
            "vector": vector,
            "total_count": len(values),
            "start": start,
            "values": list(values[start : start + limit]),
        }

    @_translate_errors
    def load_case(self, path: str) -> dict[str, Any]:
        self._require_mutable()
        case = self._require_instance().project.load_case(path=path)
        return {"id": case.id, "name": str(getattr(case, "name", ""))}

    @_translate_errors
    def import_summary_case(self, path: str) -> dict[str, Any]:
        self._require_mutable()
        summary_case = self._require_instance().project.import_summary_case(path)
        return {"id": summary_case.id, "name": str(getattr(summary_case, "name", ""))}

    @_translate_errors
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
            raise ValueError(
                "filename must be a simple file name inside the configured export directory."
            )
        self.export_directory.mkdir(parents=True, exist_ok=True)
        output_path = (self.export_directory / filename).resolve()
        if output_path.parent != self.export_directory:
            raise ValueError(
                "The export path must be inside the configured export directory."
            )
        case = self._get_case(case_id)
        temporary_path = None
        try:
            with tempfile.NamedTemporaryFile(
                mode="w",
                encoding="utf-8",
                dir=self.export_directory,
                delete=False,
            ) as output_file:
                temporary_path = Path(output_file.name)
                for chunk in case.grid_property_async(
                    property_type,
                    property_name,
                    time_step,
                    grid_index,
                    porosity_model,
                ):
                    for value in chunk.values:
                        output_file.write(f"{value}\n")
            temporary_path.replace(output_path)
        except Exception:
            if temporary_path is not None:
                temporary_path.unlink(missing_ok=True)
            raise
        return str(output_path)

    @_translate_errors
    def export_snapshots(
        self,
        prefix: str = "",
        content_type: str = "ALL",
        plot_file_format: str = "PNG",
        width: int = -1,
        height: int = -1,
    ) -> dict[str, Any]:
        self._require_mutable()
        if prefix and Path(prefix).name != prefix:
            raise ValueError("prefix must not contain a directory path.")
        self.export_directory.mkdir(parents=True, exist_ok=True)
        snapshot_directory = Path(
            tempfile.mkdtemp(prefix="snapshots-", dir=self.export_directory)
        )
        try:
            self._require_instance().project.export_snapshots(
                content_type=content_type,
                plot_file_format=plot_file_format,
                export_folder=str(snapshot_directory),
                prefix=prefix,
                width=width,
                height=height,
            )
        except Exception:
            shutil.rmtree(snapshot_directory)
            raise
        exported_files = sorted(
            path for path in snapshot_directory.iterdir() if path.is_file()
        )
        return {
            "export_directory": str(snapshot_directory),
            "files": [str(path) for path in exported_files[:_MAX_PAGE_SIZE]],
            "file_count": len(exported_files),
        }

    @_translate_errors
    def project_resource(self) -> str:
        import json

        return json.dumps(
            {"cases": self.list_cases(), "summary_cases": self.list_summary_cases()}
        )

    @_translate_errors
    def case_resource(self, case_id: int) -> str:
        import json

        return json.dumps(self.case_info(case_id))

    def _get_case(self, case_id: int) -> Any:
        case = self._require_instance().project.case(case_id)
        if case is None:
            raise ValueError(f"No grid case with id {case_id} exists in the project.")
        return case

    def _get_summary_case(self, case_id: int) -> Any:
        summary_case = self._require_instance().project.summary_case(case_id)
        if summary_case is None:
            raise ValueError(
                f"No summary case with id {case_id} exists in the project."
            )
        return summary_case

    def _require_instance(self) -> Instance:
        if self.instance is None:
            raise RuntimeError("ResInsight is not connected; call connect first.")
        return self.instance

    def _remember_pdm_object(self, pdm_object: Any) -> str:
        object_id = str(pdm_object.address())
        self._pdm_objects[object_id] = pdm_object
        return object_id

    def _get_pdm_object(self, object_id: str) -> Any:
        pdm_object = self._pdm_objects.get(object_id)
        if pdm_object is None:
            raise ValueError(
                "Unknown PDM object id; call list_pdm_objects to discover valid ids."
            )
        return pdm_object

    def _serialize_value(self, value: Any, depth: int = 0) -> Any:
        if value is None or isinstance(value, (bool, int, str)):
            return value
        if isinstance(value, float):
            return value if math.isfinite(value) else None
        if isinstance(value, enum.Enum):
            return self._serialize_value(value.value, depth + 1)
        address = getattr(value, "address", None)
        if callable(address):
            return {
                "object_id": self._remember_pdm_object(value),
                "class_keyword": value.pb2_object().class_keyword,
            }
        if depth >= 5:
            return f"<{type(value).__name__}>"
        if isinstance(value, (list, tuple)):
            values = [self._serialize_value(item, depth + 1) for item in value[:100]]
            if len(value) > 100:
                return {
                    "values": values,
                    "total_count": len(value),
                    "truncated": True,
                }
            return values
        if isinstance(value, dict):
            items = list(value.items())
            return {
                str(key): self._serialize_value(item, depth + 1)
                for key, item in items[:100]
            }
        return f"<{type(value).__name__}>"

    def _require_mutable(self) -> None:
        if self.read_only:
            raise PermissionError("This MCP server is in read-only mode.")


def create_server(read_only: bool = False, export_directory: str | None = None) -> Any:
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
    server.tool()(service.list_pdm_objects)
    server.tool()(service.pdm_get_fields)
    if not read_only:
        server.tool()(service.load_case)
        server.tool()(service.import_summary_case)
        server.tool()(service.export_grid_property)
        server.tool()(service.export_snapshots)
        server.tool()(service.pdm_set_field)
        server.tool()(service.pdm_call_method)
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
