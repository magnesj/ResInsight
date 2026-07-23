import { test, expect } from "@playwright/test";
import path from "node:path";
import { fileURLToPath } from "node:url";
import { AutomationClient, PdmObject, View } from "../src/automationClient.js";

// Reference UI workflow from issue #993: create a cell range filter, verify it
// exists, manipulate it, verify the visible cells update, delete it, and verify
// the filter is gone and the view updates.

const RANGE_FILTER_KEYWORD = "CellRangeFilter";

function fieldValue(object: PdmObject, name: string): string | undefined {
  return object.fields?.find((field) => field.name === name)?.value;
}

// The workflow needs a grid view. Use the one that is already open, otherwise import a
// model so the test is self-contained. Relative paths are resolved by the server against
// the directory ResInsight was started from, so build an absolute one.
const repositoryRoot = path.resolve(path.dirname(fileURLToPath(import.meta.url)), "../../..");
const fallbackModel = (
  process.env.RESINSIGHT_TEST_EGRID ??
  path.join(repositoryRoot, "TestModels/Case_with_10_timesteps/Real0/BRUGGE_0000.EGRID")
).replace(/\\/g, "/");

async function ensureGridView(client: AutomationClient): Promise<View> {
  const existing = await client.views();
  if (existing.length > 0) return existing[0];

  await client.command(`loadCase(path="${fallbackModel}")`);
  await client.command("createView(caseId=0)");

  const views = await client.views();
  expect(views.length, `Could not open a grid view from ${fallbackModel}`).toBeGreaterThan(0);
  return views[0];
}

test("manipulating a cell range filter updates the visible cell count", async ({ request }) => {
  const client = new AutomationClient(request);

  const views = await client.views();
  test.skip(views.length === 0, "Open a project with a 3D grid view before running this test.");
  const view = views[0];

  const project = await client.project();
  const rangeFilters = AutomationClient.findAllByKeyword(project, RANGE_FILTER_KEYWORD);
  test.skip(
    rangeFilters.length === 0,
    "Add a cell range filter to the active view before running this test.",
  );
  const rangeFilter = rangeFilters[0];

  // Verify the filter is part of the project tree.
  expect(rangeFilter.address).toBeTruthy();

  const baseline = await client.visibleCellCount(view.id);
  const originalWidthI = fieldValue(rangeFilter, "CellCountI") ?? "1";

  try {
    // Manipulate the filter so it covers a single cell slice in the I direction.
    await client.setField(rangeFilter.address, "CellCountI", "1");

    const afterManipulation = await client.visibleCellCount(view.id);
    expect(afterManipulation.visibleCellCount).not.toBe(baseline.visibleCellCount);
  } finally {
    // Restore the original extent so the test is repeatable.
    await client.setField(rangeFilter.address, "CellCountI", originalWidthI);
  }
});

// The reference workflow from issue #993, driven through the tree selection and the
// command features the context menus use. Cell range filters are created and deleted by
// features rather than by command-file verbs, so each step selects the object it acts on
// first, exactly as a user would.
test("reference workflow: create, manipulate and delete a cell range filter end-to-end", async ({
  request,
}) => {
  const client = new AutomationClient(request);
  const view = await ensureGridView(client);

  // 1. Create a range filter on the cell filter collection of the view.
  const viewObject = await client.object(view.address!);
  const collection = AutomationClient.findByKeyword(viewObject, "CellFilterCollection");
  expect(collection, "The view has no cell filter collection").toBeDefined();

  await client.setSelection(collection!.address);
  const created = await client.invokeFeature("RicNewCellRangeFilterFeature");

  // 2. Verify the filter is created. Creating features select what they made.
  const filter = created.selection.find((object) => object.keyword === RANGE_FILTER_KEYWORD);
  expect(filter, "No cell range filter was created").toBeDefined();

  // 3. Manipulate the filter and verify the visible cells update. A new filter covers the
  //    whole grid, so narrowing it to a single I slice must reduce the visible cells.
  const before = await client.visibleCellCount(view.id);
  await client.setField(filter!.address, "CellCountI", "1");
  const after = await client.visibleCellCount(view.id);
  expect(after.visibleCellCount).toBeLessThan(before.visibleCellCount);

  // 4. Delete the filter and verify it is gone and the view updates.
  await client.setSelection(filter!.address);
  await client.invokeFeature("RicDeleteItemFeature");

  const project = await client.project();
  const remaining = AutomationClient.findAllByKeyword(project, RANGE_FILTER_KEYWORD);
  expect(remaining.find((f) => f.address === filter!.address)).toBeUndefined();

  const restored = await client.visibleCellCount(view.id);
  expect(restored.visibleCellCount).toBe(before.visibleCellCount);
});
