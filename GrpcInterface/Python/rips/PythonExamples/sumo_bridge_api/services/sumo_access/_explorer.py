"""Shared Sumo Explorer wiring.

A single cached Explorer instance is reused across requests so we don't
re-authenticate on every call. The case-lookup helper centralizes error
handling so individual accessors don't need to repeat it.
"""

from __future__ import annotations

import logging
import os
from functools import lru_cache

from fmu.sumo.explorer import Explorer
from fmu.sumo.explorer.objects import Case

logger = logging.getLogger("sumo_bridge.sumo_access")

SUMO_ENV = os.environ.get("SUMO_ENV", "prod")


@lru_cache(maxsize=1)
def get_explorer() -> Explorer:
    """Return a process-wide cached Explorer instance."""
    logger.info("Creating fmu-sumo Explorer (env=%s)", SUMO_ENV)
    return Explorer(env=SUMO_ENV)


def get_case_by_uuid(case_uuid: str) -> Case:
    """Look up a Sumo case by uuid.

    Raises LookupError if the case cannot be found; routers translate this
    into an HTTP 404.
    """
    try:
        return get_explorer().get_case_by_uuid(case_uuid)
    except Exception as exc:  # fmu-sumo raises a variety of error types
        raise LookupError(f"Case '{case_uuid}' not found: {exc}") from exc
