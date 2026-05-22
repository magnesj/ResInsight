"""
Sumo Bridge API
===============

A small FastAPI service that wraps `fmu-sumo-explorer` and exposes a clean REST
interface for ResInsight (or any other client) to consume.

The goal is to keep all Sumo / Elasticsearch knowledge inside this service so
ResInsight only speaks plain REST.

Run:
    pip install -r requirements.txt
    uvicorn main:app --host 127.0.0.1 --port 8000 --reload

Then open http://127.0.0.1:8000/docs for interactive Swagger UI.
"""

from __future__ import annotations

import logging
import os
from functools import lru_cache
from typing import Any

from fastapi import FastAPI, HTTPException, Query, Response
from fastapi.middleware.cors import CORSMiddleware
from fmu.sumo.explorer import Explorer

logger = logging.getLogger("sumo_bridge")
logging.basicConfig(level=logging.INFO)


app = FastAPI(
    title="Sumo Bridge API",
    description="Local REST bridge over fmu-sumo-explorer for ResInsight.",
    version="0.1.0",
)

# CORS — allow local clients (ResInsight, browsers, notebooks) to call us.
app.add_middleware(
    CORSMiddleware,
    allow_origins=["*"],
    allow_methods=["*"],
    allow_headers=["*"],
)


# ---------------------------------------------------------------------------
# Explorer wiring
# ---------------------------------------------------------------------------

SUMO_ENV = os.environ.get("SUMO_ENV", "prod")


@lru_cache(maxsize=1)
def get_explorer() -> Explorer:
    """Lazily build a single Explorer instance.

    Cached so we don't re-authenticate on every request.
    """
    logger.info("Creating fmu-sumo Explorer (env=%s)", SUMO_ENV)
    return Explorer(env=SUMO_ENV)


def _get_case(case_id: str):
    try:
        return get_explorer().get_case_by_uuid(case_id)
    except Exception as exc:
        raise HTTPException(status_code=404, detail=f"Case '{case_id}' not found: {exc}")


# ---------------------------------------------------------------------------
# Health / meta
# ---------------------------------------------------------------------------

@app.get("/health", tags=["meta"])
def health() -> dict[str, Any]:
    return {"status": "ok", "sumo_env": SUMO_ENV}


# ---------------------------------------------------------------------------
# Cases
# ---------------------------------------------------------------------------

@app.get("/cases", tags=["cases"])
def list_cases(
    asset: str | None = Query(default=None, description="Filter by asset/field, e.g. 'Drogon'"),
    name: str | None = Query(default=None, description="Exact case name match"),
) -> list[dict[str, Any]]:
    """List Sumo cases, optionally filtered by asset and/or name."""
    explorer = get_explorer()
    cases = explorer.cases.filter(asset=asset) if asset else explorer.cases
    result = []
    for c in cases:
        if name is not None and c.name != name:
            continue
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


@app.get("/cases/{case_id}", tags=["cases"])
def get_case(case_id: str) -> dict[str, Any]:
    case = _get_case(case_id)
    return {
        "id": case.uuid,
        "name": case.name,
        "asset": getattr(case, "asset", None),
        "field": getattr(case, "field", None),
        "status": getattr(case, "status", None),
        "user": getattr(case, "user", None),
    }


# ---------------------------------------------------------------------------
# Ensembles / realizations
# ---------------------------------------------------------------------------

@app.get("/cases/{case_id}/ensembles", tags=["ensembles"])
def list_ensembles(case_id: str) -> list[dict[str, Any]]:
    case = _get_case(case_id)
    return [
        {
            "name": e.name,
            "realization_ids": [int(r) for r in e.realizationids],
        }
        for e in case.ensembles
    ]


@app.get("/cases/{case_id}/ensembles/{ensemble_name}/realizations", tags=["ensembles"])
def list_realizations(case_id: str, ensemble_name: str) -> list[int]:
    case = _get_case(case_id)
    for e in case.ensembles:
        if e.name == ensemble_name:
            return [int(r) for r in e.realizationids]
    raise HTTPException(status_code=404, detail=f"Ensemble '{ensemble_name}' not found")


# ---------------------------------------------------------------------------
# Summary vectors (tables tagged as 'summary')
# ---------------------------------------------------------------------------

_SUMMARY_METADATA_COLUMNS = {"DATE", "REAL", "ENSEMBLE", "ITER"}


def _find_ensemble(case, ensemble_name: str):
    for e in case.ensembles:
        if e.name == ensemble_name:
            return e
    raise HTTPException(status_code=404, detail=f"Ensemble '{ensemble_name}' not found")


@app.get("/cases/{case_id}/ensembles/{ensemble_name}/summary-vectors", tags=["summary"])
def list_summary_vectors(case_id: str, ensemble_name: str) -> dict[str, Any]:
    """Return summary vector column names for an ensemble's summary table."""
    case = _get_case(case_id)
    ensemble = _find_ensemble(case, ensemble_name)

    tables = ensemble.tables.filter(tagname="summary")
    if not tables:
        raise HTTPException(status_code=404, detail="No summary tables for ensemble")

    table = tables[0]
    vectors = [c for c in table.columns if c.upper() not in _SUMMARY_METADATA_COLUMNS]
    return {
        "case_id": case_id,
        "ensemble": ensemble_name,
        "vector_count": len(vectors),
        "vectors": vectors,
    }


# ---------------------------------------------------------------------------
# Surfaces
# ---------------------------------------------------------------------------

@app.get("/cases/{case_id}/surfaces", tags=["surfaces"])
def list_surfaces(
    case_id: str,
    iteration: str = Query(default="iter-0"),
    name: str | None = Query(default=None),
    tagname: str | None = Query(default=None),
) -> list[dict[str, Any]]:
    case = _get_case(case_id)
    surfaces = case.surfaces.filter(iteration=iteration)
    if name:
        surfaces = surfaces.filter(name=name)
    if tagname:
        surfaces = surfaces.filter(tagname=tagname)
    return [
        {
            "name": s.name,
            "tagname": s.tagname,
            "iteration": getattr(s, "iteration", iteration),
            "realization": getattr(s, "realization", None),
        }
        for s in surfaces
    ]


@app.get("/cases/{case_id}/surfaces/data", tags=["surfaces"])
def get_surface_data(
    case_id: str,
    name: str = Query(..., description="Surface name"),
    tagname: str = Query(..., description="Surface tagname"),
    iteration: str = Query(default="iter-0"),
    realization: int | None = Query(default=None),
):
    """Return the raw surface blob (e.g. an irap binary)."""
    case = _get_case(case_id)
    surfaces = case.surfaces.filter(name=name, tagname=tagname, iteration=iteration)
    if realization is not None:
        surfaces = surfaces.filter(realization=realization)
    if not surfaces:
        raise HTTPException(status_code=404, detail="Surface not found")
    blob = surfaces[0].blob
    return Response(content=bytes(blob), media_type="application/octet-stream")


# ---------------------------------------------------------------------------
# Grids (3D)
# ---------------------------------------------------------------------------

@app.get("/cases/{case_id}/grids", tags=["grids"])
def list_grids(
    case_id: str,
    iteration: str = Query(default="iter-0"),
) -> list[dict[str, Any]]:
    case = _get_case(case_id)
    grids = case.cube_collection if hasattr(case, "cube_collection") else case.grids
    grids = grids.filter(iteration=iteration)
    return [
        {
            "name": g.name,
            "tagname": getattr(g, "tagname", None),
            "iteration": getattr(g, "iteration", iteration),
            "realization": getattr(g, "realization", None),
        }
        for g in grids
    ]


@app.get("/cases/{case_id}/grids/{grid_name}/data", tags=["grids"])
def get_grid_data(
    case_id: str,
    grid_name: str,
    iteration: str = Query(default="iter-0"),
    realization: int | None = Query(default=None),
):
    case = _get_case(case_id)
    grids = case.grids.filter(name=grid_name, iteration=iteration)
    if realization is not None:
        grids = grids.filter(realization=realization)
    if not grids:
        raise HTTPException(status_code=404, detail="Grid not found")
    blob = grids[0].blob
    return Response(content=bytes(blob), media_type="application/octet-stream")
