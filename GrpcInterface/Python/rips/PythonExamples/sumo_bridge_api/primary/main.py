"""Sumo Bridge API — FastAPI application.

Setup:
    pip install fastapi uvicorn fmu-sumo-explorer

Run (from the parent ``PythonExamples`` directory):
    uvicorn sumo_bridge_api.primary.main:app --host 0.0.0.0 --port 8000 --reload

Docs:
    http://localhost:8000/docs
"""

from __future__ import annotations

import logging

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware

from .routers.explore.router import router as explore_router
from .routers.polygons.router import router as polygons_router
from .routers.timeseries.router import router as timeseries_router

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

app.include_router(explore_router)
app.include_router(timeseries_router)
app.include_router(polygons_router)
