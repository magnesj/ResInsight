import os
import xmlrpc.client

import pytest


def test_spix_server_responds():
    port = os.environ.get("RESINSIGHT_SPIX_PORT")
    if not port:
        pytest.skip(
            "RESINSIGHT_SPIX_PORT not set; ResInsight must be launched with --spix-port <n>"
        )

    proxy = xmlrpc.client.ServerProxy(f"http://localhost:{port}/")
    # getErrors() takes no widget path, so this verifies the RPC layer
    # is alive without depending on any specific widget's objectName.
    errors = proxy.getErrors()
    assert isinstance(errors, list)
