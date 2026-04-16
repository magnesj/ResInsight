"""
Import a SUMO ensemble into ResInsight using the fmu-sumo Python package.

This script demonstrates how to use the fmu-sumo package to access SUMO metadata
and then import the ensemble into ResInsight for visualization and analysis.

The fmu-sumo package provides a high-level API for accessing SUMO data, replacing
the fragile direct Elastic search queries used by ResInsight's built-in SUMO connector.

Prerequisites:
    pip install fmu-sumo rips

Usage:
    python sumo_ensemble_import.py
"""

# Load ResInsight Processing Server Client Library
import rips

# Load the fmu-sumo Explorer for metadata access
from fmu.sumo.explorer import Explorer


def import_sumo_ensemble(
    asset_name: str,
    case_name: str,
    iteration_name: str,
) -> None:
    """Import a SUMO ensemble into ResInsight.

    Uses fmu-sumo to access SUMO metadata (cases, ensembles, vector names,
    realization IDs) and then creates the corresponding ensemble in ResInsight.
    The actual summary data is fetched lazily by ResInsight when plots are created.

    Arguments:
        asset_name (str): The SUMO asset (field) name, e.g. "Drogon"
        case_name (str): The SUMO case name to search for
        iteration_name (str): The ensemble/iteration name within the case
    """

    # Connect to ResInsight instance
    resinsight = rips.Instance.find()
    project = resinsight.project

    # Connect to SUMO using fmu-sumo Explorer
    # The Explorer handles authentication automatically
    sumo = Explorer(env="prod")

    # Find cases matching the given asset and case name
    cases = sumo.cases.filter(asset=asset_name)
    matching_cases = [c for c in cases if c.name == case_name]

    if not matching_cases:
        print(f"No case found with name '{case_name}' for asset '{asset_name}'")
        return

    sumo_case = matching_cases[0]
    print(f"Found SUMO case: {sumo_case.name} (id: {sumo_case.uuid})")

    # Find the ensemble/iteration within the case
    iterations = sumo_case.iterations
    matching_iterations = [it for it in iterations if it.name == iteration_name]

    if not matching_iterations:
        available = [it.name for it in iterations]
        print(f"Iteration '{iteration_name}' not found. Available: {available}")
        return

    iteration = matching_iterations[0]
    print(f"Found iteration: {iteration.name}")

    # Get realization IDs from the iteration
    realization_ids = [str(r) for r in iteration.realizations]
    print(f"Found {len(realization_ids)} realizations")

    # Find summary tables within the iteration
    summary_tables = iteration.tables.filter(tagname="summary")

    if not summary_tables:
        print("No summary tables found for this iteration")
        return

    summary_table = summary_tables[0]

    # Get vector names (column names) from the summary table
    # Exclude metadata columns (DATE, REAL, ENSEMBLE, etc.)
    metadata_columns = {"DATE", "REAL", "ENSEMBLE", "ITER"}
    vector_names = [
        col for col in summary_table.column_names if col.upper() not in metadata_columns
    ]
    print(f"Found {len(vector_names)} summary vectors")

    # Import the ensemble into ResInsight
    # This creates a SummaryEnsembleSumo object that will lazily load
    # the actual parquet data when plots are created
    ensemble = project.import_summary_ensemble_sumo(
        case_id=sumo_case.uuid,
        case_name=sumo_case.name,
        ensemble_name=iteration.name,
        vector_names=vector_names,
        realization_ids=realization_ids,
    )

    if ensemble is not None:
        print(f"Successfully imported SUMO ensemble: {ensemble.name}")
    else:
        print("Failed to import SUMO ensemble")


if __name__ == "__main__":
    # Example: import a Drogon ensemble from SUMO
    import_sumo_ensemble(
        asset_name="Drogon",
        case_name="Drogon_AHM_2023-02-01",
        iteration_name="iter-0",
    )
