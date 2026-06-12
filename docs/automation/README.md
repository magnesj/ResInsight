# ResInsight UI Automation Framework

This directory defines the unified UI automation framework requested in
[#993](https://github.com/magnesj/ResInsight/issues/993) and recommended in
[#994](https://github.com/magnesj/ResInsight/pull/994). One automation surface
serves two use cases:

1. deterministic UI workflow tests in CI, and
2. UI event simulation for AI tooling.

## Architecture

| Layer | Choice |
| ----- | ------ |
| Automation transport | Localhost-only HTTP API exposed from ResInsight using **Qt HTTP Server** (`qthttpserver`), targeting **Qt 6.6.3** |
| API contract | **OpenAPI 3.0** specification ([`openapi.yaml`](openapi.yaml)) — the source of truth |
| Test runner / client | **Playwright** ([`Testing/UiAutomation`](../../Testing/UiAutomation)) |

The HTTP server binds to `127.0.0.1` only. It is intended for use on the same
machine as the running ResInsight instance.

## Building with the automation server

The server is compiled in only when explicitly enabled:

```
cmake -DRESINSIGHT_ENABLE_UI_AUTOMATION=ON ...
```

This requires the `Qt6::HttpServer` module to be available in the Qt
installation (shipped with Qt 6.4 and later). When the option is off the server
code is excluded from the build and has no runtime footprint.

## Running

When built with the option enabled, start ResInsight with:

```
ResInsight --automationserver [port]
```

The default port is `8080`. The server prints the listening port on startup.

## The OpenAPI specification

[`openapi.yaml`](openapi.yaml) is the contract. It is used to:

* document the surface for both human and AI consumers,
* generate / validate typed clients, and
* keep the C++ implementation and the Playwright tests in agreement.

## Tests

See [`Testing/UiAutomation`](../../Testing/UiAutomation) for the Playwright
project and the cell-range-filter reference workflow.
