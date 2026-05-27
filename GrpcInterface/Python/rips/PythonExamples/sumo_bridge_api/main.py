"""Sumo Bridge API entrypoint.

Thin shim so existing tooling that runs ``uvicorn main:app`` (from inside
this folder) or ``uvicorn sumo_bridge_api.main:app`` (from the parent
``PythonExamples`` folder) keeps working. The real app lives under
:mod:`sumo_bridge_api.primary.main`.
"""

from __future__ import annotations

import sys
from pathlib import Path

if __package__:
    # Loaded as ``sumo_bridge_api.main`` — relative import works.
    from .primary.main import app
else:
    # Loaded as a top-level ``main`` module (e.g. ``uvicorn main:app``
    # from inside the sumo_bridge_api folder). Make the parent directory
    # importable so the package can be found by absolute name.
    _parent_dir = str(Path(__file__).resolve().parent.parent)
    if _parent_dir not in sys.path:
        sys.path.insert(0, _parent_dir)
    from sumo_bridge_api.primary.main import app  # noqa: E402

__all__ = ["app"]
