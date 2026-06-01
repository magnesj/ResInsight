import { test, expect } from "@playwright/test";
import { AutomationClient } from "../src/automationClient.js";

// Smoke tests that exercise the read-only endpoints. They require a running
// ResInsight instance (started with --automationserver) but make no assumptions
// about the loaded project beyond what each test checks.

test("health endpoint reports the application is alive", async ({ request }) => {
  const client = new AutomationClient(request);
  const health = await client.health();

  expect(health.status).toBe("ok");
  expect(health.application).toBe("ResInsight");
  expect(health.version).toBeTruthy();
});

test("project endpoint returns the project root", async ({ request }) => {
  const client = new AutomationClient(request);
  const project = await client.project();

  expect(project.address).toBeTruthy();
  expect(project.keyword).toBeTruthy();
});

test("views endpoint and visible cell count are consistent", async ({ request }) => {
  const client = new AutomationClient(request);
  const views = await client.views();

  for (const view of views) {
    const count = await client.visibleCellCount(view.id);
    expect(count.viewId).toBe(view.id);
    expect(count.visibleCellCount).toBeGreaterThanOrEqual(0);
    if (count.totalCellCount !== undefined) {
      expect(count.visibleCellCount).toBeLessThanOrEqual(count.totalCellCount);
    }
  }
});
