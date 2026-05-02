import os
import time
import xmlrpc.client

import pytest


def test_main_window_addressable():
    """Verifies Spix can locate RiuMainWindow by objectName.

    Assumes ResInsight was launched with --spix-port <n>.

    This is the first test that depends on widget naming. If it fails,
    the regression is most likely in the objectName seeding rather
    than in Spix itself.
    """
    port = os.environ.get("RESINSIGHT_SPIX_PORT")
    if not port:
        pytest.skip("RESINSIGHT_SPIX_PORT not set")

    proxy = xmlrpc.client.ServerProxy(f"http://localhost:{port}/")

    # Let the main window finish showing before probing it.
    time.sleep(1.0)

    assert proxy.existsAndVisible("RiuMainWindow"), (
        "RiuMainWindow not found by Spix — check that "
        'RiuMainWindow::RiuMainWindow() calls setObjectName("RiuMainWindow")'
    )
