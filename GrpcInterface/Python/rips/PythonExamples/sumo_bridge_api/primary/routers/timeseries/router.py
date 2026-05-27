"""Timeseries / summary router.

Exposes endpoints for discovering and (eventually) fetching summary
timeseries data from Sumo. All Sumo Explorer interactions are delegated
to ``SummaryAccess`` in the service layer.
"""

from __future__ import annotations

from fastapi import APIRouter, HTTPException, Path

from ....services.sumo_access import SummaryAccess

from .schemas import VectorInfo

router = APIRouter(tags=["timeseries"])


@router.get("/cases/{case_uuid}/ensembles/{ensemble_name}/vectors")
async def get_available_vectors(
    case_uuid: str = Path(description="Sumo case uuid"),
    ensemble_name: str = Path(description="Ensemble name"),
) -> list[VectorInfo]:
    """List available summary vector names for the given case + ensemble."""
    access = SummaryAccess.from_case_uuid(case_uuid, ensemble_name)
    try:
        names = await access.get_available_vectors_async()
    except LookupError as exc:
        raise HTTPException(status_code=404, detail=str(exc)) from exc
    return [VectorInfo(name=n) for n in names]
