# Plan: Move well path visibility into the view, remove the global well path checkbox

Reference: #14768 "Add well path in view" (fixes #14764). That PR adds `RimWellPathInView` /
`RimWellPathInViewCollection` (a per-view mirror of `RimWellPathCollection`, modeled on the
Polygon/PolygonInView pattern) and wires it into `RivWellPathPartMgr::isWellPathEnabled()` and
`RimGeneric3dView::computeDomainBoundingBox()` as an *additional* visibility gate. It is explicitly
**not yet integrated into any view's tree/UI** and the legacy global checkbox
(`RimWellPath::m_showWellPath`, exposed via `objectToggleField()`) is left in place and still
checked everywhere. This document plans the remaining work: finish the UI integration, make the
per-view checkbox the single source of truth, remove the global checkbox, and audit every other
consumer of the global flag (including the Python/pdm-scripting surface).

## 1. Current state (as of PR #14768)

* `RimWellPathInView` / `RimWellPathInViewCollection` exist and sync from
  `RimWellPathCollection` via `RimNestedMirrorCollectionInView`, same as
  `RimPolygonInView(Collection)`.
* `RimGridView` and `RimGeneric3dView` own an `m_wellPathInViewCollection` field and add it to
  `defineUiTreeOrdering()` — so it **is** visible in the project tree today, as a sibling node next
  to the existing global "Well Paths" node, not a replacement for it.
* `RivWellPathPartMgr::isWellPathEnabled()` now additionally calls
  `isWellPathVisibleInView()`, but keeps the old check
  `wellPathCollection->wellPathVisibility() == ALL_ON && !m_rimWellPath->showWellPath()`.
* `RimWellPath::objectToggleField()` still returns `&m_showWellPath` — the per-well checkbox in the
  global "Well Paths" collection is still live and still the thing most code paths read.
* New-item seeding (`createItemInView`) and a one-time migration in
  `RimWellPathInViewCollection::initAfterRead()` (gated on project file version
  `2026.09.2`) copy `RimWellPath::showWellPath()` into the per-view checked state, so existing
  projects keep their current visibility the first time the mirror is created.

## 2. Target end state

* The **only** user-facing well path visibility checkbox is the per-view one, under each view's
  "Well Paths" node (`RimWellPathInView::m_isChecked`, inherited from `RimCheckableNamedObject`).
* The global "Well Paths" collection (and `RimWellPath` itself) no longer exposes a checkbox in the
  tree. `RimWellPathCollection` keeps `isActive` (collection-level, already the
  `objectToggleField()` for the collection) and `wellPathVisibility` (`FORCE_ALL_OFF` /
  `ALL_ON` / `FORCE_ALL_ON`, a project-wide override) — both are genuinely global concepts and stay.
* `RimWellPath::m_showWellPath` is kept internally (not removed from the class) purely as a
  migration source for old project files, is hidden from the UI, and is **not** read by any
  rendering or export/calculation code path anymore — everything goes through
  `RimWellPathInViewCollection::isWellPathVisible()` (per view) or is re-scoped to not need a
  "visible" concept at all (see §4).

This mirrors exactly how Polygons ended up: `RimPolygonCollection` has no `objectToggleField()`
override and no per-polygon global checkbox; visibility lives solely in `RimPolygonInView`.

## 3. Phased implementation plan

### Phase A — Finish view-tree integration (builds on #14768)
1. Confirm/verify `RimEclipseView`, `RimGeoMechView`, `RimGridView`, `RimGeneric3dView` all add
   `wellPathInViewCollection()` to `defineUiTreeOrdering()` next to (not instead of) the existing
   `wellCollection()`/global node — already done in #14768 for Eclipse/GeoMech/Generic3d.
2. Double check `RimSeismicView` / `Rim2dIntersectionView` / contour map views: they only picked up
   the `#include "RiaViewDefines.h"` header move, not an actual `m_wellPathInViewCollection` — decide
   per view type whether well paths are rendered there at all; if yes, they need the same wiring as
   `RimGridView`/`RimGeneric3dView` (they already derive from one of those, so check whether this is
   automatic via inheritance or needs an override).
3. Add a toolbar/context "sync visibility to new wells" affordance if needed — new wells added to
   the project must appear checked-by-default in every existing view's mirror
   (`RimWellPathCollection::updateViewTreeItems()` already triggers
   `updateFromWellPathCollection()` on add/delete, so default-checked-on-add should already work via
   `createItemInView`; verify with a manual/unit test).

### Phase B — Switch all consumers from the global flag to the per-view flag
`RimWellPath::showWellPath()` is currently read in (non-test) code at:

| File | Call site | View context available? | Planned change |
|---|---|---|---|
| `ModelVisualization/RivWellPathPartMgr.cpp:177,196` | `isWellPathEnabled`, `appendStaticFracturePartsToModel` | Yes, `m_rimView` | Drop the `showWellPath()` half of the `ALL_ON && !showWellPath()` check once the global checkbox is gone; keep the `wellPathVisibility()` FORCE_ALL_ON/OFF project-wide override. |
| `ProjectDataModel/RimGeneric3dView.cpp:193` | `computeDomainBoundingBox` | Yes, `this` | Drop `showWellPath()`, keep the already-added `isWellPathVisible()` check. |
| `ProjectDataModel/WellMeasurement/RimWellMeasurementInViewCollection.cpp:93` | `visibleMeasurementsForWellPath` (`linkWellVisibility`) | Yes — this is itself a per-view collection | Replace `wellPath->showWellPath()` with `firstAncestorOfType<Rim3dView>()->wellPathInViewCollection()->isWellPathVisible(wellPath)`. |
| `Commands/CompletionExportCommands/RicExportCompletionsForVisibleWellPathsFeature.cpp:120,134` | "Export Completions for Visible Well Paths" tree feature | Yes — can resolve the active/selected view | Resolve the active `Rim3dView` (e.g. via `RiaApplication::instance()->activeReservoirView()` or the selected item's ancestor view) and filter with its `wellPathInViewCollection()->isWellPathVisible()`. Needs product confirmation of which view is "the" view when multiple are open. |
| `ProjectDataModelCommands/RimcEclipseCase.cpp:819` (`RimEclipseCase_exportCompletions::execute`, Python `export_completions`) | Default well-path list when `well_paths` argument is empty | **No** — this is a case-level pdm command, not bound to a view | **Python API behavior change** — see §4/§5. Needs a product decision: either require an explicit `well_path_names`/view argument, default to "all well paths" (drop the filter), or add a new `view` parameter. |
| `ProjectDataModel/RimEclipseCase.cpp:453` | `computeAndGetVirtualPerforationTransmissibilities` | No — per-case, not per-view | Likely drop the `showWellPath()` filter entirely (include all well paths with perforations); flag as a behavior change to confirm with domain owners, since this feeds simulation-adjacent calculations, not just 3D rendering. |
| `ProjectDataModel/Completions/RimCompletionCellIntersectionCalc.cpp:83` | `calculateCompletionTypeResult` | No — per-case result calculation | Same as above: either drop the filter (include all) or thread a view through; confirm whether this "completion type" cell result is meant to reflect what is currently visible in 3D or the full well set. |
| `Commands/RicDeleteSubItemsFeature.cpp:186` | "Delete Unchecked Items" on `RimWellPathCollection` | No — acts on the global collection | The whole feature loses meaning for well paths once there's no global checked state. Remove the `RimWellPathCollection` branch from this feature (or repoint it at "well paths not visible in the currently active view", which is a weaker and more surprising semantic — prefer removal). |

4. After the above are migrated, remove `RimWellPath::objectToggleField()`'s override (or make it
   return `nullptr`) so the per-well checkbox disappears from the global "Well Paths" tree node.
   Keep `m_showWellPath` as a plain hidden field (already `setUiHidden(true)`) for migration only;
   keep `setShowWellPath()`/`showWellPath()` accessors since the migration code in
   `RimWellPathInViewCollection` still calls them.

### Phase C — Tests
1. Rewrite `ApplicationLibCode/FeatureTests/Tests/RicToggleItemsFeature-Test.cpp`
   (`RicToggleItemsFeatureTest` fixture) to toggle the per-view `RimWellPathInView` items instead of
   `RimWellPath` directly — needs a `Rim3dView` in the test fixture (there is already
   `RiaFeatureTestModelBuilder`; check whether it creates a view, otherwise extend it).
2. Update/extend `RimWellPathInViewCollection-Test.cpp` (added in #14768) with cases for multi-view
   independence of the *rendering* path (already covered at the collection level) plus the new
   consumers from Phase B once they're converted (measurement linking, export-completions default
   selection).
3. Add a regression test confirming that `RimWellPath::objectToggleField()` no longer drives
   visibility (e.g. assert it returns `nullptr`, or assert the tree item for a well path under the
   global collection is not checkable) to prevent regressions.

### Phase D — Python / pdm-scripting API evaluation
Findings from auditing the scripting surface:
* `RimWellPath::m_showWellPath` is declared with plain `CAF_PDM_InitField` (not
  `CAF_PDM_InitScriptableField`) — **it is not exposed to the Python API today**, so removing the
  global checkbox does not remove any existing scriptable field. No `rips` Python class attribute
  needs deprecating for this reason alone.
* `RimWellPathCollection::wellPathVisibility` (`GlobalWellPathVisibility`,
  FORCE_ALL_OFF/ALL_ON/FORCE_ALL_ON) is likewise not scriptable — unaffected, stays as the one
  remaining global visibility knob.
* `RimWellPathInView` / `RimWellPathInViewCollection` are currently constructed with plain
  `CAF_PDM_InitObject` (not `CAF_PDM_InitScriptableObject...`), same as the Polygon/Surface in-view
  equivalents they're modeled on (`RimPolygonInView`, `RimSurfaceInView` are also not scriptable
  today) — so there is currently **no** Python-scriptable way to read or set per-view well path
  (or polygon/surface) visibility, before or after this change. This is a pre-existing gap in the
  in-view mirror pattern, not something introduced by this plan.
* The one real Python-facing behavior change is **`RimEclipseCase_exportCompletions`**
  (`eclipse_case.export_completions(...)` in `rips`): today, when called without an explicit
  `well_paths` list, it silently defaults to "all well paths with the global checkbox checked"
  (`RimcEclipseCase.cpp:819`). After this change there is no global checkbox, so this default must
  be redefined. Needs a decision, to be reflected in the `rips` docstring/changelog:
  * Option 1 (simplest, recommended): default to *all* well paths in the project when none are
    given, dropping the implicit visibility filter — scripts that want a subset already pass
    `well_paths` explicitly.
  * Option 2: add an optional `view` parameter and default to that view's
    `wellPathInViewCollection()->visibleWellPathsInView()`, preserving today's "visible wells"
    semantics but making it explicit and view-scoped.
* Action items for this phase:
  1. Decide between Option 1/2 above for `RimEclipseCase_exportCompletions` and implement.
  2. Update the Python API docs/changelog (`GrpcInterface/Python`, scripting reference docs) for the
     `export_completions` default-selection change.
  3. Optionally (separate, smaller follow-up, not required for parity): make
     `RimWellPathInView`/`RimWellPathInViewCollection` scriptable (`CAF_PDM_InitScriptableObject`,
     `CAF_PDM_InitScriptableField` on `m_isChecked`/name) so Python scripts can toggle per-view well
     path visibility going forward — if desired, do this consistently together with
     `RimPolygonInView`/`RimSurfaceInView` rather than as a one-off for well paths.
  4. Add/update a `rips` Python test (under the Python test suite) covering the new
     `export_completions` default-selection behavior, since the current implicit behavior has no
     visible test coverage in the repo today (confirm by searching `GrpcInterface/Python` tests).

### Phase E — Project file compatibility / migration window
1. `RimWellPathInViewCollection::initAfterRead()` already gates its one-time
   checkbox-sync-from-legacy-field migration on
   `proj->isProjectFileVersionEqualOrOlderThan("2026.09.2")`. Confirm this version constant matches
   the actual release where the per-view checkbox is first shipped to users (update if the
   checkbox-removal lands in a later version than 14768).
   `createItemInView()`'s unconditional seeding from `showWellPath()` already handles "never had a
   mirror before" projects; the `initAfterRead` branch only matters for the narrow window of
   projects saved with 14768 merged but before the checkbox was removed.
2. Plan a follow-up cleanup PR, at least one release later, to delete
   `RimWellPath::m_showWellPath`/`showWellPath()`/`setShowWellPath()` entirely once the migration
   window owners are comfortable no supported project file still needs it as a seed value.

### Phase F — Documentation
1. Update any user-facing docs/screenshots that reference "Show Well Path" under the global Well
   Paths folder.
2. Add a changelog entry describing the UX change (well path visibility is now per-view) and the
   `export_completions` Python behavior change.

## 4. Open product questions (need a decision before Phase B/D can be finished)

1. `RimEclipseCase::computeAndGetVirtualPerforationTransmissibilities` and
   `RimCompletionCellIntersectionCalc::calculateCompletionTypeResult` are per-*case* calculations
   with no natural view context. Once there's no global "visible" flag, should these:
   (a) include all well paths with perforations unconditionally, or
   (b) be re-scoped to accept a view and use that view's per-view visibility?
   Recommendation: (a), since these are data/result calculations rather than 3D-view rendering, and
   a single case can be shown in multiple views with different per-view well path visibility.
2. `RicExportCompletionsForVisibleWellPathsFeature` needs an explicit notion of "the current view"
   when none/only a collection is selected in the tree — confirm which view should be used if
   several views of the same case are open (active view vs. first view vs. prompt the user).
3. Confirm whether `RicDeleteSubItemsFeature`'s "Delete Unchecked Items" should simply drop support
   for well paths, or be redefined against per-view visibility (recommend dropping — "unchecked
   in which view?" is not a good UX fit for a bulk-delete action on the global collection).

## 5. Suggested PR breakdown

1. PR 1 (this repo's #14768, already open): land `RimWellPathInView`/`RimWellPathInViewCollection`
   plumbing, additive only, no behavior change for users.
2. PR 2: Phase A finish + Phase B rendering/measurement-linking call sites (items with a view
   context) + Phase C tests for those.
3. PR 3: Resolve open questions in §4, migrate the remaining non-view call sites (export feature,
   Python `export_completions`, perforation/completion-type calculations), update Python docs.
4. PR 4: Remove `RimWellPath::objectToggleField()` override (the actual "remove the checkbox" step),
   update/remove `RicToggleItemsFeature-Test.cpp` well path cases and
   `RicDeleteSubItemsFeature`'s well path branch.
5. PR 5 (later release): delete the legacy `m_showWellPath` field once the migration window has
   elapsed.
