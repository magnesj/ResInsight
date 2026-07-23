# ResInsight UI Automation Tests (Playwright)

Playwright project that drives the ResInsight automation HTTP API. The API
contract lives in [`docs/automation/openapi.yaml`](../../docs/automation/openapi.yaml).

## Prerequisites

* Node.js 18+
* A ResInsight build configured with `-DRESINSIGHT_ENABLE_UI_AUTOMATION=ON`

## Setup

```bash
cd Testing/UiAutomation
npm install
npx playwright install
```

## Running ResInsight with the automation server

```bash
ResInsight --automationserver 8080 --project <some_project.rsp>
```

The server binds to `127.0.0.1` only. Override the URL the tests use with the
`RESINSIGHT_AUTOMATION_URL` environment variable (default
`http://127.0.0.1:8080/api/v1`).

## Running the tests

```bash
npm test            # all tests
npm run test:smoke  # read-only smoke tests only
```

## Test layout

| File | Purpose |
| ---- | ------- |
| `tests/smoke.spec.ts` | Read-only checks of `/health`, `/project`, `/views` and `/views/{id}/visibleCellCount`. |
| `tests/cell-range-filter.spec.ts` | The cell-range-filter reference workflow from issue #993, creating and deleting the filter through command features. Imports a model from `TestModels` when no grid view is open. |
| `tests/import-case.spec.ts` | Importing an EGRID case through `/cases`. Override the model with `RESINSIGHT_TEST_EGRID`. |
| `tests/features.spec.ts` | The `/features` allow list, and the rejection of features that are not allowed or do not apply. |
| `tests/selection.spec.ts` | Selecting objects in the project tree through `/selection`. |
| `src/automationClient.ts` | Typed client whose shapes mirror the OpenAPI spec. |

## Importing a grid case

```bash
curl -X POST http://127.0.0.1:8080/api/v1/cases \
  -H "Content-Type: application/json" \
  -d '{"path": "/path/to/MODEL.EGRID", "createView": true}'
```

The response reports the new case id and the ids of the views created for it:

```json
{"caseId": 0, "viewIds": [0]}
```

Paths are resolved by the application, so prefer absolute paths. Pass
`"createView": false` to import the case without opening a view.

## Selecting objects

Many ResInsight commands act on the current project tree selection. Select an
object by its address, as reported in the `/project` tree:

```bash
curl -X PUT http://127.0.0.1:8080/api/v1/selection \
  -H "Content-Type: application/json" \
  -d '{"address": "2525203073120"}'
```

`GET /selection` returns the objects currently selected. Addresses are only
valid for the lifetime of the object, so read them from `/project` in the same
test rather than hard-coding them.

## Invoking commands

`POST /features` triggers a command feature, the action behind a context menu
entry. Features act on the tree selection, so select first:

```bash
curl -X PUT http://127.0.0.1:8080/api/v1/selection \
  -H "Content-Type: application/json" -d '{"address": "2835701592096"}'

curl -X POST http://127.0.0.1:8080/api/v1/features \
  -H "Content-Type: application/json" \
  -d '{"commandId": "RicNewCellRangeFilterFeature"}'
```

Features that create an object select it, so the response reports the new object
and its address.

Only features on an allow list can be invoked. Many of the several hundred
features open modal dialogs, and a dialog raised from a request handler blocks
the application with nobody present to dismiss it. `GET /features` lists what is
enabled, together with whether each one applies to the current selection. To
enable another feature, check that its implementation opens no dialog and add it
to `allowedCommandFeatures()` in `RiaAutomationServer.cpp`.
