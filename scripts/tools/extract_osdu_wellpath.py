#!/usr/bin/env python3
"""
Standalone OSDU well trajectory inspector.

Fetches a wellbore record and a wellbore-trajectory record from OSDU, dumps
the SpatialLocation block from each, downloads the trajectory parquet and
prints the absolute UTM coordinates after applying the wellbore surface
origin.

This is independent of ResInsight. Use it to validate what ResInsight's
OSDU importer should produce.

By default, configuration is read from ResInsight's local files:
    ~/.resinsight/osdu_config.json   server, dataPartitionId, ...
    ~/.resinsight/osdu_token.json    token (bearer access token)

CLI flags override the values from those files. You can also use
--config-file / --token-file to point at alternate locations, or set
OSDU_SERVER / OSDU_PARTITION / OSDU_TOKEN as env-var overrides.

Requirements:
    pip install requests pandas pyarrow

Example (uses ~/.resinsight/osdu_*.json automatically):
    python extract_osdu_wellpath.py \\
        --wellbore-id "opendes:master-data--Wellbore:abcd" \\
        --trajectory-id "opendes:work-product-component--WellboreTrajectory:efgh"
"""

from __future__ import annotations

import argparse
import io
import json
import os
import sys
from pathlib import Path
from typing import Optional, Tuple

import requests


DEFAULT_CONFIG_FILE = Path.home() / ".resinsight" / "osdu_config.json"
DEFAULT_TOKEN_FILE = Path.home() / ".resinsight" / "osdu_token.json"


def _headers(partition: str, token: str, accept: str) -> dict:
    return {
        "Authorization": f"Bearer {token}",
        "Data-Partition-Id": partition,
        "Accept": accept,
    }


def load_json(path: Path) -> dict:
    if not path.is_file():
        return {}
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except json.JSONDecodeError as exc:
        print(f"Warning: {path} is not valid JSON ({exc}); ignoring.", file=sys.stderr)
        return {}


def fetch_storage_record(server: str, partition: str, token: str, record_id: str) -> dict:
    url = f"{server.rstrip('/')}/api/storage/v2/records/{record_id}"
    resp = requests.get(url, headers=_headers(partition, token, "application/json"), timeout=30)
    resp.raise_for_status()
    return resp.json()


def fetch_trajectory_parquet(server: str, partition: str, token: str, trajectory_id: str) -> bytes:
    url = f"{server.rstrip('/')}/api/os-wellbore-ddms/ddms/v3/wellboretrajectories/{trajectory_id}/data"
    resp = requests.get(url, headers=_headers(partition, token, "application/x-parquet"), timeout=120)
    resp.raise_for_status()
    return resp.content


def extract_surface_origin(spatial_location: dict) -> Tuple[Optional[float], Optional[float], str]:
    ingested = spatial_location.get("AsIngestedCoordinates", {}) or {}
    crs = ingested.get("persistableReferenceCrs", "")
    features = ingested.get("features", []) or []
    if features:
        coords = features[0].get("geometry", {}).get("coordinates", [])
        if len(coords) >= 2:
            return float(coords[0]), float(coords[1]), crs
    return None, None, crs


def main() -> int:
    parser = argparse.ArgumentParser(description="Inspect an OSDU wellbore trajectory.")
    parser.add_argument("--config-file", type=Path, default=DEFAULT_CONFIG_FILE,
                        help=f"OSDU config json (default: {DEFAULT_CONFIG_FILE})")
    parser.add_argument("--token-file", type=Path, default=DEFAULT_TOKEN_FILE,
                        help=f"OSDU token json (default: {DEFAULT_TOKEN_FILE})")
    parser.add_argument("--server", default=os.environ.get("OSDU_SERVER"))
    parser.add_argument("--partition", default=os.environ.get("OSDU_PARTITION"))
    parser.add_argument("--token", default=os.environ.get("OSDU_TOKEN"))
    parser.add_argument("--wellbore-id", required=True)
    parser.add_argument("--trajectory-id", required=True)
    parser.add_argument("--rows", type=int, default=5, help="Rows to print from head and tail of the parquet.")
    args = parser.parse_args()

    config = load_json(args.config_file)
    token_data = load_json(args.token_file)

    server = args.server or config.get("server")
    partition = args.partition or config.get("dataPartitionId")
    token = args.token or token_data.get("token")

    missing = [n for n, v in [("server", server), ("partition", partition), ("token", token)] if not v]
    if missing:
        parser.error(
            f"Missing required configuration: {', '.join(missing)}.\n"
            f"Looked in: config={args.config_file}, token={args.token_file}.\n"
            "Provide via CLI flag, env var, or update those files."
        )

    print(f"Server   : {server}")
    print(f"Partition: {partition}")
    print()

    print(f"=== Wellbore: {args.wellbore_id} ===")
    wellbore = fetch_storage_record(server, partition, token, args.wellbore_id)
    spatial = wellbore.get("data", {}).get("SpatialLocation", {}) or {}
    print(json.dumps(spatial, indent=2))
    easting, northing, crs = extract_surface_origin(spatial)
    print(f"Surface origin: easting={easting} northing={northing} crs={crs!r}")

    print()
    print(f"=== Trajectory: {args.trajectory_id} ===")
    trajectory = fetch_storage_record(server, partition, token, args.trajectory_id)
    traj_spatial = trajectory.get("data", {}).get("SpatialLocation", {}) or {}
    print(json.dumps(traj_spatial, indent=2))

    print()
    print("=== Trajectory parquet ===")
    parquet_bytes = fetch_trajectory_parquet(server, partition, token, args.trajectory_id)
    print(f"Downloaded {len(parquet_bytes)} bytes")

    try:
        import pandas as pd
        import pyarrow.parquet as pq
    except ImportError:
        print("pandas / pyarrow not installed; skipping parquet inspection.", file=sys.stderr)
        return 0

    table = pq.read_table(io.BytesIO(parquet_bytes))
    df = table.to_pandas()
    print(f"Columns: {list(df.columns)}")
    print(f"Rows:    {len(df)}")

    cols = [c for c in ("MD", "TVD", "X", "Y") if c in df.columns]
    if cols:
        print()
        print("Head:")
        print(df[cols].head(args.rows).to_string(index=False))
        print()
        print("Tail:")
        print(df[cols].tail(args.rows).to_string(index=False))

    if easting is not None and northing is not None and {"X", "Y"}.issubset(df.columns):
        print()
        print("Absolute UTM after applying surface origin (E + X, N + Y):")
        first, last = df.iloc[0], df.iloc[-1]
        print(f"  first: E={first['X'] + easting:.3f}, N={first['Y'] + northing:.3f}")
        print(f"  last : E={last['X']  + easting:.3f}, N={last['Y']  + northing:.3f}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
