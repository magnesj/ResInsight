"""Convenience methods for working with summary ensembles from Python.

The methods are added to the ``SummaryEnsemble`` class, which is used
for both summary ensembles and summary groups. Use the ``is_ensemble`` flag to
pick the ensembles.
"""

import datetime
import logging
import math
from typing import Dict, List, Optional, Sequence

from .exception import RipsError
from .pdmobject import add_method
from .resinsight_classes import SummaryEnsemble

logger = logging.getLogger(__name__)

DATE_COLUMN = "DATE"
REALIZATION_COLUMN = "REAL"


def _to_datetime(time_stamp: int) -> datetime.datetime:
    return datetime.datetime.fromtimestamp(
        time_stamp, tz=datetime.timezone.utc
    ).replace(tzinfo=None)


@add_method(SummaryEnsemble)
def vector_values(
    self: SummaryEnsemble, address: str, time_resolution: str = "NONE"
) -> Dict[int, Dict[str, list]]:
    """Read one summary vector for all realizations in the ensemble.

    Arguments:
        address (str): Summary vector address, for example "FOPT" or "WOPT:A1"
        time_resolution (str): "NONE" returns the original time steps of each
            realization. Use "DAY", "WEEK", "MONTH", "QUARTER", "HALFYEAR" or
            "YEAR" to resample all realizations to a common time axis.

    Returns:
        dict: Realization (summary case) id mapped to a dictionary with the
        keys "time_steps" (seconds since epoch) and "values".
    """
    result = {}
    for summary_case in self.summary_cases():
        data = summary_case.resample_values(address, time_resolution)
        result[summary_case.id] = {
            "time_steps": list(data.time_steps),
            "values": list(data.values),
        }
    return result


@add_method(SummaryEnsemble)
def to_dataframe(
    self: SummaryEnsemble,
    addresses: Sequence[str],
    time_resolution: str = "MONTH",
):
    """Read summary vectors for all realizations into a pandas DataFrame.

    The DataFrame is in the long format used by FMU tools, with one row per
    realization and time step. The columns are "DATE", "REAL" (the summary
    case id of the realization) and one column per requested address.

    Arguments:
        addresses (list[str]): Summary vector addresses
        time_resolution (str): Resampling period, see vector_values(). Use
            "NONE" to keep the original time steps of each realization.

    Returns:
        pandas.DataFrame
    """
    try:
        import pandas as pd
    except ImportError as exc:
        raise RipsError("pandas is required to create a DataFrame") from exc

    frames = []
    for summary_case in self.summary_cases():
        columns: Dict[str, list] = {}
        time_steps: Optional[List[int]] = None
        for address in addresses:
            data = summary_case.resample_values(address, time_resolution)
            if time_steps is None:
                time_steps = list(data.time_steps)
            elif list(data.time_steps) != time_steps:
                raise RipsError(
                    f"Vector '{address}' in case {summary_case.id} has different "
                    "time steps than the other vectors"
                )
            columns[address] = list(data.values)

        if time_steps is None:
            continue

        frame = pd.DataFrame(columns)
        frame.insert(0, REALIZATION_COLUMN, summary_case.id)
        frame.insert(0, DATE_COLUMN, [_to_datetime(t) for t in time_steps])
        frames.append(frame)

    if not frames:
        return pd.DataFrame(columns=[DATE_COLUMN, REALIZATION_COLUMN, *addresses])

    return pd.concat(frames, ignore_index=True)


@add_method(SummaryEnsemble)
def set_vectors_from_dataframe(
    self: SummaryEnsemble,
    dataframe,
    units: Optional[Dict[str, str]] = None,
) -> None:
    """Write DataFrame columns back to the realizations as new summary vectors.

    The DataFrame must have the columns "DATE" and "REAL" as created by
    to_dataframe(). All other columns are written as summary vectors.

    - The rows of each realization are sorted by date, as operations in Python
      can change the row order.
    - The dates must be identical to the original time steps of the
      realization, otherwise an error is raised. Create the DataFrame with
      time_resolution="NONE" to use the original time steps.
    - A warning is logged if a vector already exists in the realization.
      Vectors from the original summary file cannot be overwritten.
    - NaN values are written as NaN.

    Arguments:
        dataframe (pandas.DataFrame): Data to write
        units (dict): Optional mapping from column name to unit
    """
    units = units or {}

    for column in (DATE_COLUMN, REALIZATION_COLUMN):
        if column not in dataframe.columns:
            raise RipsError(f"The DataFrame is missing the '{column}' column")

    vector_names = [
        c for c in dataframe.columns if c not in (DATE_COLUMN, REALIZATION_COLUMN)
    ]
    cases_by_id = {case.id: case for case in self.summary_cases()}

    for real, group in dataframe.groupby(REALIZATION_COLUMN):
        summary_case = cases_by_id.get(int(real))
        if summary_case is None:
            raise RipsError(f"Realization {real} is not part of the ensemble")

        group = group.sort_values(DATE_COLUMN)
        dates = [_to_datetime(t) for t in summary_case.available_time_steps().values]
        if list(group[DATE_COLUMN].dt.to_pydatetime()) != dates:
            raise RipsError(
                f"The dates for realization {real} do not match the time steps "
                "of the summary case"
            )

        existing = set(summary_case.available_addresses().values)
        for name in vector_names:
            values = [float(v) for v in group[name]]
            if any(math.isnan(v) for v in values):
                logger.warning(
                    "Vector '%s' in realization %s contains NaN values", name, real
                )
            if name in existing:
                logger.warning(
                    "Vector '%s' already exists in realization %s", name, real
                )
            summary_case.set_summary_values(name, units.get(name, ""), values)
