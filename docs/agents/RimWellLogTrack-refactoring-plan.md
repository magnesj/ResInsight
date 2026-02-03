# RimWellLogTrack Refactoring Plan

## Overview

Extract 4 logical field groupings from RimWellLogTrack (3,864 lines, 37 fields) into separate settings classes, following established codebase patterns.

## Current State
- **RimWellLogTrack.h**: 380 lines
- **RimWellLogTrack.cpp**: 3,484 lines
- **Fields**: 37 PdmFields
- **Dependents**: 69 files include this header

## Extraction Order

| Phase | New Class | Fields | Risk |
|-------|-----------|--------|------|
| 1 | RimWellLogPropertyAxisSettings | 11 | LOW |
| 2 | RimWellLogFormationSettings | 10 | MEDIUM |
| 3 | RimWellLogRegionAnnotationSettings | 6 | MEDIUM |
| 4 | RimWellLogWellPathAttributeSettings | 10 | MEDIUM |

---

## Phase 1: RimWellLogPropertyAxisSettings (LOW RISK)

### Fields to Extract
```
m_isPropertyAxisEnabled
m_visiblePropertyValueRangeMin
m_visiblePropertyValueRangeMax
m_isAutoScalePropertyValuesEnabled
m_isPropertyLogarithmicScaleEnabled
m_invertPropertyValueAxis
m_propertyValueAxisGridVisibility
m_propertyAxisMinAndMaxTicksOnly
m_explicitTickIntervalsPropertyValueAxis
m_majorTickIntervalPropertyAxis
m_minorTickIntervalPropertyAxis
```

### Files to Create
- `ApplicationLibCode/ProjectDataModel/WellLog/RimWellLogPropertyAxisSettings.h`
- `ApplicationLibCode/ProjectDataModel/WellLog/RimWellLogPropertyAxisSettings.cpp`

### Changes to RimWellLogTrack
1. Add child field: `caf::PdmChildField<RimWellLogPropertyAxisSettings*> m_propertyAxisSettings`
2. Rename original fields with `_OBSOLETE` suffix
3. Set `.xmlCapability()->disableIO()` on obsolete fields
4. Add migration in `initAfterRead()` using `isProjectFileVersionEqualOrOlderThan()`
5. Keep public API methods as delegating wrappers

### Public API to Keep (as delegation)
```cpp
void setLogarithmicScale(bool enable);
bool isLogarithmicScale() const;
void setTickIntervals(double major, double minor);
void setMinAndMaxTicksOnly(bool enable);
void setPropertyValueAxisGridVisibility(...);
void enablePropertyAxis(bool enable);
void setVisiblePropertyValueRange(double min, double max);
void setAutoScalePropertyValuesEnabled(bool enabled);
```

---

## Phase 2: RimWellLogFormationSettings (MEDIUM RISK)

### Fields to Extract
```
m_formationSource
m_formationCase
m_formationTrajectoryType
m_formationWellPathForSourceCase
m_formationWellPathForSourceWellPath
m_formationSimWellName
m_formationBranchIndex
m_formationBranchDetection
m_formationLevel
m_showformationFluids
```

### Enums to Move
- `TrajectoryType` (WELL_PATH, SIMULATION_WELL)
- `FormationSource` (CASE, WELL_PICK_FILTER)

### Files to Create
- `ApplicationLibCode/ProjectDataModel/WellLog/RimWellLogFormationSettings.h`
- `ApplicationLibCode/ProjectDataModel/WellLog/RimWellLogFormationSettings.cpp`

### Public API to Keep (as delegation)
```cpp
void setFormationWellPath(RimWellPath* wellPath);
RimWellPath* formationWellPath() const;
void setFormationSimWellName(const QString& simWellName);
void setFormationCase(RimCase* rimCase);
void setFormationTrajectoryType(TrajectoryType trajectoryType);
void setFormationBranchDetection(bool branchDetection);
void setFormationBranchIndex(int branchIndex);
```

---

## Phase 3: RimWellLogRegionAnnotationSettings (MEDIUM RISK)

### Fields to Extract
```
m_regionAnnotationType
m_regionAnnotationDisplay
m_colorShadingLegend
m_colorShadingTransparency
m_showRegionLabels
m_regionLabelFontSize
```

### Files to Create
- `ApplicationLibCode/ProjectDataModel/WellLog/RimWellLogRegionAnnotationSettings.h`
- `ApplicationLibCode/ProjectDataModel/WellLog/RimWellLogRegionAnnotationSettings.cpp`

### Public API to Keep (as delegation)
```cpp
void setAnnotationType(RiaDefines::RegionAnnotationType annotationType);
void setAnnotationDisplay(RiaDefines::RegionDisplay annotationDisplay);
void setAnnotationTransparency(int percent);
void setColorShadingLegend(RimColorLegend* colorLegend);
void setShowRegionLabels(bool on);
```

---

## Phase 4: RimWellLogWellPathAttributeSettings (MEDIUM RISK)

### Fields to Extract
```
m_showWellPathAttributes
m_showWellPathCompletions
m_showWellPathComponentsBothSides
m_showWellPathComponentLabels
m_wellPathAttributesInLegend
m_wellPathCompletionsInLegend
m_wellPathComponentSource
m_wellPathAttributeCollection
m_overburdenHeight
m_underburdenHeight
```

### Files to Create
- `ApplicationLibCode/ProjectDataModel/WellLog/RimWellLogWellPathAttributeSettings.h`
- `ApplicationLibCode/ProjectDataModel/WellLog/RimWellLogWellPathAttributeSettings.cpp`

### Public API to Keep (as delegation)
```cpp
void setShowWellPathAttributes(bool on);
void setShowWellPathAttributesInLegend(bool on);
void setShowWellPathCompletionsInLegend(bool on);
void setShowBothSidesOfWell(bool on);
void setWellPathAttributesSource(RimWellPath* wellPath);
void setOverburdenHeight(double overburdenHeight);
void setUnderburdenHeight(double underburdenHeight);
```

---

## Implementation Pattern (for each phase)

### 1. Create New Settings Class
```cpp
// Header
class RimWellLogPropertyAxisSettings : public caf::PdmObject
{
    CAF_PDM_HEADER_INIT;
public:
    RimWellLogPropertyAxisSettings();
    // Getters and setters
    void uiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering);
protected:
    void defineUiOrdering(...) override;
    void fieldChangedByUi(...) override;
private:
    caf::PdmField<...> m_fields;
};
```

### 2. Add Child Field in RimWellLogTrack
```cpp
caf::PdmChildField<RimWellLogPropertyAxisSettings*> m_propertyAxisSettings;
```

### 3. Initialize in Constructor
```cpp
CAF_PDM_InitFieldNoDefault(&m_propertyAxisSettings, "PropertyAxisSettings", "");
m_propertyAxisSettings = new RimWellLogPropertyAxisSettings();
m_propertyAxisSettings.uiCapability()->setUiTreeChildrenHidden(true);
```

### 4. Mark Original Fields Obsolete
```cpp
CAF_PDM_InitField(&m_isPropertyAxisEnabled_OBSOLETE, "IsPropertyAxisEnabled", true, "");
m_isPropertyAxisEnabled_OBSOLETE.xmlCapability()->disableIO();
```

### 5. Migrate in initAfterRead
```cpp
void RimWellLogTrack::initAfterRead()
{
    if (RimProject::current()->isProjectFileVersionEqualOrOlderThan("2026.04"))
    {
        m_propertyAxisSettings->setEnabled(m_isPropertyAxisEnabled_OBSOLETE());
        // ... migrate other fields
    }
}
```

---

## Reference Files (established patterns)
- `ApplicationLibCode/ProjectDataModel/Completions/RimWellPathCompletionSettings.cpp` - obsolete field migration
- `ApplicationLibCode/ProjectDataModel/Completions/RimMswCompletionParameters.h` - settings class structure
- `ApplicationLibCode/ProjectDataModel/Completions/RimFishbones.cpp` - proxy fields and legacy migration

---

## Files to Modify
- `ApplicationLibCode/ProjectDataModel/WellLog/RimWellLogTrack.h`
- `ApplicationLibCode/ProjectDataModel/WellLog/RimWellLogTrack.cpp`
- `ApplicationLibCode/ProjectDataModel/WellLog/CMakeLists_files.cmake`

---

## Verification

After each phase:
1. Build with Ninja from `build` folder
2. Run unit tests: `ResInsight --unittest`
