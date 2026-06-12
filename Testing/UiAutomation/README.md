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
| `tests/cell-range-filter.spec.ts` | The cell-range-filter reference workflow from issue #993. The end-to-end create/delete lifecycle is marked `fixme` until the corresponding `/commands` vocabulary is available. |
| `src/automationClient.ts` | Typed client whose shapes mirror the OpenAPI spec. |
