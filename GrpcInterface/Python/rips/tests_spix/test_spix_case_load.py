import os
import time
import xmlrpc.client

import pytest


def test_case_load_produces_no_errors():
    """First Spix test beyond the smoke check.

    Assumes ResInsight was launched with:

        --spix-port <n> --case <TestModels>/TEST10K_FLT_LGR_NNC/TEST10K_FLT_LGR_NNC.EGRID

    Verifies the full GUI case-load path completes without errors.
    The gRPC rips tests do not exercise this path — they use the
    command server rather than the GUI's load codepath.
    """
    port = os.environ.get("RESINSIGHT_SPIX_PORT")
    if not port:
        pytest.skip(
            "RESINSIGHT_SPIX_PORT not set; ResInsight must be launched "
            "with --spix-port <n> --case <path>/TEST10K_FLT_LGR_NNC.EGRID"
        )

    proxy = xmlrpc.client.ServerProxy(f"http://localhost:{port}/")

    # No widget signal to wait on yet (Qt objectNames not seeded);
    # let the async case-load settle before reading errors.
    time.sleep(2.0)

    errors = proxy.getErrors()
    assert errors == [], f"Spix reported errors after case load: {errors}"
