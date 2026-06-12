import { test, expect } from "@playwright/test";
import { AutomationClient, PdmObject } from "../src/automationClient.js";

// Reference UI workflow from issue #993: create a cell range filter, verify it
// exists, manipulate it, verify the visible cells update, delete it, and verify
// the filter is gone and the view updates.

const RANGE_FILTER_KEYWORD = "CellRangeFilter";

function fieldValue(object: PdmObject, name: string): string | undefined {
  return object.fields?.find((field) => field.name === name)?.value;
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

// The full create/delete lifecycle depends on command vocabulary for creating and
// removing cell range filters through POST /commands. This is the documented target
// for the framework and is enabled once those commands are available server-side.
test.fixme(
  "reference workflow: create, manipulate and delete a cell range filter end-to-end",
  async ({ request }) => {
    const client = new AutomationClient(request);

    const views = await client.views();
    const view = views[0];

    // 1. Create a range filter on the active view.
    await client.command(`createCellRangeFilter(viewId=${view.id})`);

    // 2. Verify the filter is created.
    let project = await client.project();
    let filters = AutomationClient.findAllByKeyword(project, RANGE_FILTER_KEYWORD);
    expect(filters.length).toBeGreaterThan(0);
    const filter = filters[filters.length - 1];

    // 3. Manipulate the filter and verify the visible cells update.
    const before = await client.visibleCellCount(view.id);
    await client.setField(filter.address, "CellCountI", "1");
    const after = await client.visibleCellCount(view.id);
    expect(after.visibleCellCount).not.toBe(before.visibleCellCount);

    // 4. Delete the filter and verify it is gone.
    await client.command(`deleteCellFilter(address=${filter.address})`);
    project = await client.project();
    filters = AutomationClient.findAllByKeyword(project, RANGE_FILTER_KEYWORD);
    expect(filters.find((f) => f.address === filter.address)).toBeUndefined();
  },
);
