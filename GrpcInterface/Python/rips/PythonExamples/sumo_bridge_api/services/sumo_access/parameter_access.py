"""Access class for summary / timeseries data on Sumo.

Wraps fmu-sumo-explorer summary tables so the router layer only has to
deal with simple, typed return values.
"""

from __future__ import annotations

from ._explorer import get_case_by_uuid

from fmu.sumo.explorer.objects import Table


class ParameterAccess:
    """Access parameter data for a given Sumo case + ensemble."""

    def __init__(self, case_uuid: str, ensemble_name: str) -> None:
        self._case_uuid = case_uuid
        self._ensemble_name = ensemble_name

    @classmethod
    def from_case_uuid(cls, case_uuid: str, ensemble_name: str) -> "ParameterAccess":
        return cls(case_uuid=case_uuid, ensemble_name=ensemble_name)

    async def get_parameters_blob_url_async(self) -> str:
        """Get the blob URL for the given parameter table

        The temporary solution is not optimized, so we trigger aggregation to ensure the blob URL is available, this triggers an aggregation 

        Returns the raw Azure blob URL. The caller should authenticate using
        OAuth Bearer token (same token used for Sumo API access).
        """

        case = get_case_by_uuid(self._case_uuid)

        sc_ensemble = case.filter(ensemble=self._ensemble_name)
        sc_parameters_per_real = sc_ensemble.filter(realization=True, aggregation=False).parameters

        realization_count = await sc_parameters_per_real.length_async()
        if realization_count == 0:
            raise LookupError(
                f"No parameters found for case {self._case_uuid} and ensemble {self._ensemble_name}"
            )

        sc_param_table = sc_ensemble.parameters
        try:
            parameter_agg = await sc_param_table.aggregation_async(operation="collection")
        except Exception as exp:
            raise LookupError(
                f"Parameter aggregation failed for case {self._case_uuid} and ensemble {self._ensemble_name}"
            ) from exp
        
        if not isinstance(parameter_agg, Table):
            raise LookupError("Did not get expected object type of Table for parameter aggregation")
        

        blob_url = parameter_agg.metadata["_sumo"]["blob_url"]

        return blob_url