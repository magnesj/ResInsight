"""Typed return values for the grid access layer."""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class GridInfo:
    """A grid name together with the realizations it is available for."""

    name: str
    realizations: list[int]
