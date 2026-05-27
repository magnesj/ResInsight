"""Polygons router.

Placeholder router; endpoints to be added once we wire up a
``PolygonsAccess`` accessor in the service layer.
"""

from __future__ import annotations

from fastapi import APIRouter

router = APIRouter(tags=["polygons"])

@router.get("/cases/{case_uuid}/ensembles/{ensemble_name}/polygons")