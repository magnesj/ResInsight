import os
import xmlrpc.client

import pytest


def test_spix_main_window_visible():
    port = os.environ.get("RESINSIGHT_SPIX_PORT")
    if not port:
        pytest.skip(
            "RESINSIGHT_SPIX_PORT not set; ResInsight must be launched with --spix-port <n>"
        )

    proxy = xmlrpc.client.ServerProxy(f"http://localhost:{port}/")
    assert proxy.existsAndVisible("mainWindow") is True
