from __future__ import annotations

from ._explorer import get_case_by_uuid
from .grid_types import GridInfo


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