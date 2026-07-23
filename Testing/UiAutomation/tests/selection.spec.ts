import { test, expect } from "@playwright/test";
import { AutomationClient } from "../src/automationClient.js";

// Selecting objects in the project tree. Selection is what a user does before invoking a
// context menu command, so a test needs to be able to drive it.

test("selecting an object in the project tree is reflected in the selection", async ({ request }) => {
  const client = new AutomationClient(request);

  const project = await client.project();
  const target = AutomationClient.findByKeyword(project, "WellPaths");
  test.skip(target === undefined, "Project tree does not contain a Wells collection.");

  const selected = await client.setSelection(target!.address);
  expect(selected.address).toBe(target!.address);

  const selection = await client.selection();
  expect(selection.map((object) => object.address)).toContain(target!.address);
});

test("selection survives being changed to another object", async ({ request }) => {
  const client = new AutomationClient(request);

  const project = await client.project();
  const first = AutomationClient.findByKeyword(project, "WellPaths");
  const second = AutomationClient.findByKeyword(project, "ResInsightAnalysisModels");
  test.skip(
    first === undefined || second === undefined,
    "Project tree does not contain the expected collections.",
  );

  await client.setSelection(first!.address);
  await client.setSelection(second!.address);

  const selection = await client.selection();
  const addresses = selection.map((object) => object.address);
  expect(addresses).toContain(second!.address);
  expect(addresses).not.toContain(first!.address);
});

test("selecting an unknown address reports not found", async ({ request }) => {
  const response = await request.put("selection", { data: { address: "1" } });

  expect(response.status()).toBe(404);
});
