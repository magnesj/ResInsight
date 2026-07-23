import { test, expect } from "@playwright/test";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { AutomationClient } from "../src/automationClient.js";

// Importing an Eclipse grid file. The model comes from the repository TestModels folder;
// override it with RESINSIGHT_TEST_EGRID to run against another grid.

// Paths are resolved by the server, so build an absolute one from the repository root.
const repositoryRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../../..");
const egridPath = (
  process.env.RESINSIGHT_TEST_EGRID ??
  path.join(repositoryRoot, "TestModels/Case_with_10_timesteps/Real0/BRUGGE_0000.EGRID")
).replace(/\\/g, "/");

test("importing an EGRID file creates a case with a view", async ({ request }) => {
  const client = new AutomationClient(request);

  const imported = await client.importCase(egridPath);
  expect(imported.caseId).toBeGreaterThanOrEqual(0);
  expect(imported.viewIds.length).toBeGreaterThan(0);

  // The imported case is part of the project tree.
  const project = await client.project();
  expect(AutomationClient.findAllByKeyword(project, "EclipseCase").length).toBeGreaterThan(0);

  // The reported view exists and shows cells.
  const views = await client.views();
  expect(views.map((view) => view.id)).toContain(imported.viewIds[0]);

  const count = await client.visibleCellCount(imported.viewIds[0]);
  expect(count.totalCellCount).toBeGreaterThan(0);
  expect(count.visibleCellCount).toBeGreaterThan(0);
});

test("importing without a view creates the case only", async ({ request }) => {
  const client = new AutomationClient(request);

  const imported = await client.importCase(egridPath, false);
  expect(imported.caseId).toBeGreaterThanOrEqual(0);
  expect(imported.viewIds).toEqual([]);
});

test("importing a non-existent file reports an error", async ({ request }) => {
  const response = await request.post("cases", { data: { path: "no_such_model.EGRID" } });

  expect(response.status()).toBe(400);
});
