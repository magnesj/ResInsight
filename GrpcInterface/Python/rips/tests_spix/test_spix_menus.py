import os
import time
import xmlrpc.client

import pytest


def test_main_window_menus_reachable():
    """Verifies window branding and that every seeded top-level menu is
    reachable from RiuMainWindow via its objectName.

    Exercises Spix's getStringProperty RPC against seven QMenu widgets
    plus the QMainWindow itself. QMenus parented to a menubar report
    isVisible()==False until popped up, so existsAndVisible would
    return False; getStringProperty still works because the QObject
    tree lookup ignores visibility.

    Assumes ResInsight was launched with --spix-port <n>.
    """
    port = os.environ.get("RESINSIGHT_SPIX_PORT")
    if not port:
        pytest.skip("RESINSIGHT_SPIX_PORT not set")

    proxy = xmlrpc.client.ServerProxy(f"http://localhost:{port}/")

    time.sleep(1.0)

    title = proxy.getStringProperty("RiuMainWindow", "windowTitle")
    assert "ResInsight" in title, f"Unexpected window title: {title!r}"

    for menu in (
        "FileMenu",
        "EditMenu",
        "ViewMenu",
        "WindowsMenu",
        "HelpMenu",
        "ExportMenu",
        "TestingMenu",
    ):
        name = proxy.getStringProperty(f"RiuMainWindow//{menu}", "objectName")
        assert name == menu, (
            f"Menu '{menu}' not reachable from RiuMainWindow "
            f"(getString returned {name!r}). Check setObjectName in "
            f"RiuMenuBarBuildTools or RiuMainWindow::createMenus()."
        )

    assert proxy.getErrors() == []
