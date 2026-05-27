"""Sumo access layer.

Provides accessor classes that wrap fmu-sumo-explorer and expose
typed, task-oriented methods for the router layer to consume.
"""

from .case_inventory_access import CaseInventoryAccess
from .summary_access import SummaryAccess

__all__ = ["CaseInventoryAccess", "SummaryAccess"]
