import { test, expect } from "@playwright/test";
import { AutomationClient } from "../src/automationClient.js";

// The /features endpoint invokes the command features behind the context menus. Only an
// explicit allow list is reachable, because many features open modal dialogs that would
// block the server with no user present to dismiss them.

test("the allowed features are listed with their executable state", async ({ request }) => {
  const client = new AutomationClient(request);
  const features = await client.features();

  expect(features.length).toBeGreaterThan(0);
  for (const feature of features) {
    expect(feature.commandId).toBeTruthy();
    expect(typeof feature.canExecute).toBe("boolean");
  }
  expect(features.map((feature) => feature.commandId)).toContain("RicNewCellRangeFilterFeature");
});

test("a feature outside the allow list is rejected", async ({ request }) => {
  const response = await request.post("features", {
    data: { commandId: "RicImportEclipseCaseFeature" },
  });

  expect(response.status()).toBe(403);
});

test("an unknown feature is rejected", async ({ request }) => {
  const response = await request.post("features", { data: { commandId: "NoSuchFeature" } });

  expect(response.status()).toBe(403);
});

test("a feature that does not apply to the selection is refused", async ({ request }) => {
  const client = new AutomationClient(request);

  // The Wells collection is not deletable, so the delete feature must refuse to run.
  const project = await client.project();
  const wells = AutomationClient.findByKeyword(project, "WellPaths");
  test.skip(wells === undefined, "Project tree does not contain a Wells collection.");

  await client.setSelection(wells!.address);

  const features = await client.features();
  const deleteFeature = features.find((f) => f.commandId === "RicDeleteItemFeature");
  expect(deleteFeature?.canExecute).toBe(false);

  const response = await request.post("features", { data: { commandId: "RicDeleteItemFeature" } });
  expect(response.status()).toBe(409);
});
