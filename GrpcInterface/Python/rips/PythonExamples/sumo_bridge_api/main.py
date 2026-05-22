"""
Sumo Bridge API — minimal FastAPI skeleton over fmu-sumo-explorer.

Setup:
    pip install fastapi uvicorn fmu-sumo-explorer

Run:
    uvicorn main:app --host 0.0.0.0 --port 8000 --reload

Docs:
    http://localhost:8000/docs
"""

import logging
from functools import lru_cache
import os
from typing import Any


from fastapi import FastAPI, HTTPException, Path, Query, logger
from fastapi.middleware.cors import CORSMiddleware
from fmu.sumo.explorer import Explorer
from fmu.sumo.explorer.objects import Case


logger = logging.getLogger("sumo_bridge")
logging.basicConfig(level=logging.INFO)

app = FastAPI(title="Sumo Bridge API")

# Allow ResInsight (or any local client) to call this
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)

SUMO_ENV = os.environ.get("SUMO_ENV", "prod")

@lru_cache(maxsize=1)
def get_explorer() -> Explorer:
    """Lazily build a single Explorer instance.

    Cached so we don't re-authenticate on every request.
    """
    logger.info("Creating fmu-sumo Explorer (env=%s)", SUMO_ENV)
    return Explorer(env=SUMO_ENV)

def _get_case(case_id: str) -> Case:
    try:
        return get_explorer().get_case_by_uuid(case_id)
    except Exception as exc:
        raise HTTPException(status_code=404, detail=f"Case '{case_id}' not found: {exc}") from exc


# TODO: 
# - Does not work with async?
# - Webviz has endpoint for cases info - which includes list of ensembles per case.
# - Add query param, consider sumo auth/token arg (middleware?)
# - Use pydantic basemodel for response models, to control what we return and how it is serialized.


@app.get("/assets", tags=["assets"])
def get_assets() -> list[str]:
    """List available assets."""
    assets = get_explorer().asset_names
    return assets

@app.get("/cases", tags=["cases"])
def get_cases(
    asset_name: str = Query(min_length=1, description="Asset name"),
) -> list[dict[str, Any]]:
    """List Sumo cases for given asset
    
    # TODO: 
    # Webviz uses elastic search for this endpoint
    #  - Webviz assembles case info, which also includes list of ensembles per case. To prevent new query to retrieve ensemble 
    """
    explorer = get_explorer()
    cases = explorer.cases.filter(asset=asset_name) 
    result = []
    for c in cases:
        result.append(
            {
                "id": c.uuid,
                "name": c.name,
                "asset": getattr(c, "asset", None),
                "field": getattr(c, "field", None),
                "status": getattr(c, "status", None),
                "user": getattr(c, "user", None),
            }
        )
    return result

@app.get("/cases/{case_uuid}/ensembles", tags=["ensembles"])
def get_ensembles(case_uuid: str) -> list[str]:
    """Get ensembles for a given case"""
    case = _get_case(case_uuid)
    return case.ensembles.ensemblenames

@app.get("/cases/{case_uuid}/ensembles/{ensemble_name}/realizations", tags=["ensembles"])
async def get_ensemble_realizations(
    case_uuid: str = Path(description="Sumo case uuid"),
    ensemble_name: str = Path(description="Ensemble name")
) -> list[int]:
    """Get realizations for a given case and ensemble"""
    case = _get_case(case_uuid)
    ensemble = case.filter(ensemble=ensemble_name, realization=True)
    realization_list = await ensemble.realizationids_async
    return sorted(realization_list)


