"""Access class for summary / timeseries data on Sumo.

Wraps fmu-sumo-explorer summary tables so the router layer only has to
deal with simple, typed return values.
"""

from __future__ import annotations

from ._explorer import get_case_by_uuid

# Non-vector columns that may appear in a summary table and should be filtered
# out when listing available vectors.
_SUMMARY_METADATA_COLUMNS = {"DATE", "REAL", "ENSEMBLE", "ITER"}


class SummaryAccess:
    """Access summary (timeseries) data for a given Sumo case + ensemble."""

    def __init__(self, case_uuid: str, ensemble_name: str) -> None:
        self._case_uuid = case_uuid
        self._ensemble_name = ensemble_name

    @classmethod
    def from_case_uuid(cls, case_uuid: str, ensemble_name: str) -> "SummaryAccess":
        return cls(case_uuid=case_uuid, ensemble_name=ensemble_name)

    async def get_available_vectors_async(self) -> list[str]:
        """Return the list of available summary vector names.

        Uses the summary table associated with the ensemble. Metadata
        columns (DATE, REAL, ENSEMBLE, ITER) are filtered out.
        """
        case = get_case_by_uuid(self._case_uuid)
        ensemble = case.filter(ensemble=self._ensemble_name)
        summary_tables = ensemble.tables.filter(tagname="summary")

        if len(summary_tables) == 0:
            raise LookupError(
                f"No summary tables found for ensemble '{self._ensemble_name}' "
                f"in case '{self._case_uuid}'"
            )

        table = summary_tables[0]
        columns = await table.columns_async
        return [c for c in columns if c.upper() not in _SUMMARY_METADATA_COLUMNS]
