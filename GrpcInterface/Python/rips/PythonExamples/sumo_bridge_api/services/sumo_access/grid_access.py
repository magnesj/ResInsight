from __future__ import annotations
import asyncio

from sumo_bridge_api.lib.utils.timestamp_utils import iso_str_to_date_str

from ._explorer import get_case_by_uuid
from .grid_types import GridInfo, GridPropertyInfo

from fmu.sumo.explorer.objects import CPGrid
from fmu.sumo.explorer import TimeFilter, TimeType

class GridAccess:
    """Access grid data for a given Sumo case + ensemble."""

    def __init__(self, case_uuid: str, ensemble_name: str) -> None:
        self._case_uuid = case_uuid
        self._ensemble_name = ensemble_name

    @classmethod
    def from_case_uuid(cls, case_uuid: str, ensemble_name: str) -> "GridAccess":
        return cls(case_uuid=case_uuid, ensemble_name=ensemble_name)
    
    async def get_available_grid_info_list_async(self) -> list[GridInfo]:
        """Return the list of available grids with their realizations."""
        case = get_case_by_uuid(self._case_uuid)

        grid_context = case.grids.grids.filter(ensemble=self._ensemble_name)
        if await grid_context.length_async() == 0:
            raise LookupError(
                f"No grid tables found for ensemble '{self._ensemble_name}' "
                f"in case '{self._case_uuid}'"
            )

        grid_names = await grid_context.names_async

        grid_infos: list[GridInfo] = []
        for grid_name in grid_names:
            realization_context = grid_context.filter(
                name=grid_name, realization=True
            )
            realization_ids = await realization_context.realizationids_async
            grid_infos.append(
                GridInfo(
                    name=grid_name,
                    realizations=sorted(int(r) for r in realization_ids),
                )
            )
        return grid_infos

    async def get_grid_blob_url_async(self, grid_name: str, realization: int) -> str:
        """Get the blob URL for the grid data for the given case + ensemble."""
        case = get_case_by_uuid(self._case_uuid)

        grid_context = case.grids.filter(ensemble=self._ensemble_name, name=grid_name, realization=realization)
        if await grid_context.length_async() == 0:
            raise LookupError(
                f"No grid table named '{grid_name}' found for ensemble '{self._ensemble_name}' "
                f"in case '{self._case_uuid}', and realization {realization}"
            )
        
        table_names = await grid_context.names_async
        if len(table_names) == 0:
            raise LookupError(
                f"No grid tables found in case={self._case_uuid}, ensemble={self._ensemble_name}"
            )
        if len(table_names) > 1:
            raise LookupError(
                f"Multiple grid tables found in case={self._case_uuid}, ensemble={self._ensemble_name}: {table_names=}"
            )
        
        grid_table = await grid_context.getitem_async(0)
        blob_url = grid_table.metadata["_sumo"]["blob_url"]
        return blob_url
    
    async def get_grid_properties_async(self, grid_name: str, realization: int) -> list[GridPropertyInfo]:
        """Get the properties for a grid."""
        case = get_case_by_uuid(self._case_uuid)

        grid_context = case.grids.filter(ensemble=self._ensemble_name, name=grid_name, realization=realization)
        if await grid_context.length_async() == 0:
            raise LookupError(
                f"No grid table named '{grid_name}' found for ensemble '{self._ensemble_name}' "
                f"in case '{self._case_uuid}', and realization {realization}"
            )
        
        # Expect unique grid:
        if len(grid_context) != 1:
            raise ValueError(f"Expected exactly one grid with name '{grid_name}', found {len(grid_context)}")
        
        sumo_grid_object = grid_context[0]
        if not isinstance(sumo_grid_object, CPGrid):
            raise TypeError(f"Expected CPGrid, got {type(sumo_grid_object)}")
        
        no_time_context = sumo_grid_object.grid_properties.filter(time=TimeFilter(time_type=TimeType.NONE))
        timestamp_context = sumo_grid_object.grid_properties.filter(time=TimeFilter(time_type=TimeType.TIMESTAMP))
        interval_context = sumo_grid_object.grid_properties.filter(time=TimeFilter(time_type=TimeType.INTERVAL))

        async with asyncio.TaskGroup() as tg:
            no_time_property_names_task = tg.create_task(no_time_context.names_async)
            timestamp_property_names_task = tg.create_task(timestamp_context.names_async)
            timestamp_property_timestamps_task = tg.create_task(timestamp_context.timestamps_async)
            interval_property_names_task = tg.create_task(interval_context.names_async)
            interval_property_intervals_task = tg.create_task(interval_context.intervals_async)

        no_time_property_names = no_time_property_names_task.result()
        timestamp_property_names = timestamp_property_names_task.result()
        timestamp_property_timestamps = timestamp_property_timestamps_task.result()
        interval_property_names = interval_property_names_task.result()
        interval_property_intervals = interval_property_intervals_task.result()

        property_info_arr: list[GridPropertyInfo] = []

        for property_name in no_time_property_names:
            property_info_arr.append(GridPropertyInfo(property_name=property_name, iso_date_or_interval=None))
        for property_name in timestamp_property_names:
            for timestamp in timestamp_property_timestamps:
                property_info_arr.append(
                    GridPropertyInfo(
                        property_name=property_name,
                        iso_date_or_interval=iso_str_to_date_str(timestamp),
                    )
                )
        for property_name in interval_property_names:

            for interval in interval_property_intervals:
                property_info_arr.append(
                    GridPropertyInfo(
                        property_name=property_name,
                        iso_date_or_interval=f"{iso_str_to_date_str(interval[0])}/{iso_str_to_date_str(interval[1])}",
                    )
                )

        return property_info_arr