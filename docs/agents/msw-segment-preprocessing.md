# MSW Segment Preprocessing Refactor

> **Branch:** `pre-process-msw-segments-01`

---

## Context

Currently, `RicMswTableDataTools::collectWelsegsSegment` calls `createSubSegmentMDPairs` to split segments into sub-segments on-the-fly **during export**. This means:
- WelsegsRow creation and segment subdivision are entangled
- `RicMswSegment` data (`outputMD`, `outputTVD`, `segmentNumber`) is mutated during the export pass
- `RimCustomSegmentInterval` logic lives inside the export function rather than preprocessing
- Grid-cell segments span the full cell (startMD=entry, endMD=exit) instead of using the cell midpoint

The refactor:
1. Moves all segment subdivision into a **preprocessing step** before any WelsegsRow creation
2. Changes grid-cell segment geometry to use **cell midpoint as endMD**, previous segment end as startMD
3. Applies `RimCustomSegmentInterval` (and `maxSegmentLength`) during preprocessing
4. Makes `collectWelsegsSegment` purely read-only — no mutations to `RicMswSegment`

---

## Files to Modify

| File | Changes |
|------|---------|
| `ApplicationLibCode/Commands/CompletionExportCommands/RicMswSegment.h/.cpp` | Add `setStartMD(double)` and `setStartTVD(double)` setters |
| `ApplicationLibCode/Commands/CompletionExportCommands/RicMswBranch.h/.cpp` | (No change needed — existing `insertAfterSegment` inserts *before* the found iterator, usable for preprocessing) |
| `ApplicationLibCode/Commands/CompletionExportCommands/RicWellPathExportMswTableData.h/.cpp` | Change `createWellPathSegments` geometry; add `preprocessMainBoreSegments`; call preprocessing in `extractSingleWellMswData` |
| `ApplicationLibCode/Commands/CompletionExportCommands/RicMswTableDataTools.h/.cpp` | Simplify `collectWelsegsSegment`; remove sub-segmentation loop and all mutations; change parameter to `const RicMswSegment*` |

---

## Step-by-Step Changes

### 1. `RicMswSegment` — Add setters for startMD/startTVD (DONE)

```cpp
// In RicMswSegment.h, add:
void setStartMD( double startMD );
void setStartTVD( double startTVD );
```

These are needed when preprocessing splits a segment: the last sub-segment (which inherits the completions) has its start MD/TVD updated to match the last sub-pair's start.

Also: in the **constructor**, initialize `m_outputMD = endMD` and `m_outputTVD = endTVD` so that outputMD/TVD are always valid from construction without requiring `setOutputMD` to be called during export.

### 2. `createWellPathSegments` — Use cell midpoint geometry

**Current:** each cell → segment spanning `[cellIntInfo.startMD, cellIntInfo.endMD]`

**New:** each cell → segment spanning `[prevEndMD, cellMidMD]` where:
- `cellMidMD = 0.5 * (cellIntInfo.startMD + cellIntInfo.endMD)`
- `cellMidTVD = tvdFromMeasuredDepth(wellPath, cellMidMD)`
- `prevEndMD` = end of previous segment (or `branch->startMD()` for first)

Set `outputMD = endMD` and `outputTVD = endTVD` at construction (done via constructor initialization in Step 1).

Track `prevEndMD` / `prevEndTVD` across iterations:
```cpp
double prevEndMD  = branch->startMD();
double prevEndTVD = branch->startTVD();
for ( const auto& cellIntInfo : cellSegmentIntersections )
{
    double midMD  = 0.5 * ( cellIntInfo.startMD + cellIntInfo.endMD );
    double midTVD = -wellPath->wellPathGeometry()->interpolatedPointAlongWellPath( midMD ).z();

    auto segment = std::make_unique<RicMswSegment>(
        label, prevEndMD, midMD, prevEndTVD, midTVD );

    prevEndMD  = midMD;
    prevEndTVD = midTVD;
    // ... add perforations, add to branch ...
}
```

### 3. New function `RicWellPathExportMswTableData::preprocessMainBoreSegments`

Add to `RicWellPathExportMswTableData.h` (private static):
```cpp
static void preprocessMainBoreSegments(
    gsl::not_null<RicMswBranch*>                  branch,
    const RimWellPath*                            wellPath,
    double                                        maxSegmentLength,
    const std::vector<std::pair<double, double>>& customSegmentIntervals );
```

Implementation logic:
```
For each segment in branch->segments():
    subPairs = createSubSegmentMDPairs(segment->startMD(), segment->endMD(),
                                       maxSegmentLength, customSegmentIntervals)
    if subPairs.size() > 1:
        // Insert N-1 new sub-segments before `segment` (which becomes the last)
        for i in 0 .. subPairs.size()-2:
            [subStart, subEnd] = subPairs[i]
            subTVDStart = tvdFromWellPath(wellPath, subStart)
            subTVDEnd   = tvdFromWellPath(wellPath, subEnd)
            newSeg = new RicMswSegment(label, subStart, subEnd, subTVDStart, subTVDEnd)
            branch->insertAfterSegment(segment, std::move(newSeg))
            // Note: insertAfterSegment(X, Y) inserts Y BEFORE X in the vector

        // Update the original segment to be the last sub-segment
        [lastStart, lastEnd] = subPairs.back()
        lastTVDStart = tvdFromWellPath(wellPath, lastStart)
        segment->setStartMD(lastStart)    // new setter
        segment->setStartTVD(lastTVDStart) // new setter
        // segment->endMD() and outputMD unchanged

For each child branch:
    preprocessMainBoreSegments(childBranch, childWellPath, maxSegmentLength, customSegmentIntervals)
```

**Note on `insertAfterSegment`:** The existing implementation uses `m_segments.insert(it, ...)` which inserts **before** `it`. So `insertAfterSegment(original, sub1)` places `sub1` before `original`. Calling it repeatedly for sub1, sub2, sub3 results in `[..., sub1, sub2, sub3, original, ...]` — correct ordering.

### 4. Call preprocessing in `extractSingleWellMswData`

In `RicWellPathExportMswTableData.cpp`, after the existing setup but **before** `collectWelsegsData`:

```cpp
// After: assignBranchNumbersToBranch(...)
// After: tableData is created

// Preprocess: split segments per custom intervals + max length
std::vector<std::pair<double, double>> customSegmentIntervals = mswParameters->getSegmentIntervals();
preprocessMainBoreSegments( exportInfo.mainBoreBranch(),
                             wellPath,
                             mswParameters->maxSegmentLength(),
                             customSegmentIntervals );

// Then call collectWelsegsData WITHOUT customSegmentIntervals/maxSegmentLength for main bore
RicMswTableDataTools::collectWelsegsData( tableData,
                                           exportInfo,
                                           mswParameters->maxSegmentLength(),     // still needed for valves/completions
                                           customSegmentIntervals,                 // still needed for valves/completions
                                           exportCompletionsAfterMainBoreSegments,
                                           exportDate );
```

*(The `maxSegmentLength` and `customSegmentIntervals` parameters remain in `collectWelsegsData` because `collectValveWelsegsSegment` and `collectCompletionWelsegsSegments` still use `createSubSegmentMDPairs` — those are not changed in this refactor.)*

### 5. Simplify `collectWelsegsSegment`

**Remove:**
- Call to `createSubSegmentMDPairs`
- The sub-segment loop (`for (const auto& [subStartMD, subEndMD] : segments)`)
- `segment->setOutputMD(...)`, `segment->setOutputTVD(...)`, `segment->setSegmentNumber(...)`
- `maxSegmentLength` and `customSegmentIntervals` parameters

**Change** `segment` parameter from `RicMswSegment*` to `const RicMswSegment*`

**New simplified body:**
```cpp
void RicMswTableDataTools::collectWelsegsSegment(
    RigMswTableData& tableData,
    const RicMswSegment* segment,
    const RicMswSegment* previousSegment,
    RicMswExportInfo& exportInfo,
    gsl::not_null<RicMswBranch*> branch,
    QString branchDescription,
    const std::optional<QDateTime>& exportDate )
{
    CVF_ASSERT( segment );

    double reportMD  = segment->outputMD();   // = endMD, set at construction
    double reportTVD = segment->outputTVD();  // = endTVD, set at construction

    double prevOutMD  = branch->startMD();
    double prevOutTVD = branch->startTVD();
    if ( previousSegment )
    {
        prevOutMD  = previousSegment->outputMD();
        prevOutTVD = previousSegment->outputTVD();
    }

    if ( reportMD < prevOutMD )
    {
        prevOutMD  = branch->startMD();
        prevOutTVD = branch->startTVD();
    }

    double depth = 0, length = 0;
    if ( exportInfo.lengthAndDepthText() == "INC" )
    {
        depth  = reportTVD - prevOutTVD;
        length = reportMD  - prevOutMD;
    }
    else
    {
        depth  = reportTVD;
        length = reportMD;
    }

    // ... linerDiameter, roughnessFactor, WelsegsRow creation ...
    // No mutations to segment data.
}
```

### 6. Update `collectWelsegsDataRecursively`

- Remove `maxSegmentLength` and `customSegmentIntervals` from the call to `collectWelsegsSegment`
- Move `(*segmentNumber)++` to **after** `collectWelsegsSegment` returns (instead of inside it)
- `segment->setSegmentNumber(*segmentNumber)` still happens before the call (not inside the export function)

```cpp
// In the main loop:
segment->setSegmentNumber( *segmentNumber );

collectWelsegsSegment( tableData, segment, outletSegment, exportInfo,
                       branch, branchDescription, exportDate );
( *segmentNumber )++;   // moved here from inside collectWelsegsSegment
outletSegment = segment;
```

---

## Key Invariants After Refactor

- `segment->outputMD() == segment->endMD()` for all main bore segments
- `segment->startMD() == previous_segment->endMD()` for all main bore segments (by construction)
- `collectWelsegsSegment` takes only `const RicMswSegment*` — compiler enforces no mutation
- `createSubSegmentMDPairs` is NOT called in `collectWelsegsSegment` (still used in `collectValveWelsegsSegment`, `collectCompletionWelsegsSegments`, and the new `preprocessMainBoreSegments`)

---

## What Is NOT Changed

- `collectValveWelsegsSegment` — still uses `createSubSegmentMDPairs` (valve segments, not grid-cell derived)
- `collectCompletionWelsegsSegments` — still uses `createSubSegmentMDPairs` (completion segments)
- `collectWsegvalvData`, `collectCompsegData` — unchanged
- `RicMswBranch` — no new methods needed (existing `insertAfterSegment` suffices)

---

## Verification

1. Build the project — ensure no compile errors
2. Run existing MSW export tests
3. Export an MSW well to file and compare WELSEGS table output with expected segment numbers, lengths, and depths
4. Test with a well that has `RimCustomSegmentInterval` defined — verify segment boundaries match the intervals
5. Test with `maxSegmentLength > 0` — verify sub-segments are created in preprocessing, not during export
