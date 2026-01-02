# defineEditorAttribute Migration Guide

This guide documents the migration from the old `defineEditorAttribute()` pattern to the new map-based attribute system.

## Overview

The new system moves attribute configuration from the deprecated `defineEditorAttribute()` method to either:
1. **Constructor** - for truly static attributes that never change
2. **defineUiOrdering()** - for dynamic attributes that depend on runtime state

## Benefits

- ✅ **Performance**: Set once instead of on every UI refresh (for static attrs)
- ✅ **Type Safety**: Type-safe helpers (`setAttributeInt`, `setAttributeBool`, etc.)
- ✅ **Validation**: Runtime warnings for unsupported attributes
- ✅ **Maintainability**: Cleaner code, attributes near field initialization
- ✅ **Discoverability**: All attributes visible in one place

## Migration Status

### Completed (6 files in this session)
- RimPerforationInterval
- RimPolygon
- RicCellRangeUi
- RimWellPathValve
- RimWellPathTieIn
- RimBoxIntersection

### Remaining in ApplicationLibCode (69 files)
- **Simple Sliders**: 8 files (static values)
- **Dynamic Sliders**: 12 files (runtime values)
- **Button Text**: 5 files (conditional text)
- **Complex**: 44 files (manual review needed)

## Using the Migration Helper

Run the helper script to see categorized files:

```bash
python migration_helper.py
```

This will show:
- Files grouped by complexity
- Code templates for common patterns
- Recommended migration order

## Migration Patterns

### 1. Static Slider Attributes

**Example**: Angle slider with fixed range 0-360

**Before** (defineEditorAttribute):
```cpp
void MyClass::defineEditorAttribute(const caf::PdmFieldHandle* field,
                                    QString uiConfigName,
                                    caf::PdmUiEditorAttribute* attribute)
{
    if (field == &m_azimuthAngle)
    {
        auto* attr = dynamic_cast<caf::PdmUiDoubleSliderEditorAttribute*>(attribute);
        if (attr)
        {
            attr->m_minimum = 0;
            attr->m_maximum = 360;
            attr->m_sliderTickCount = 360;
        }
    }
}
```

**After** (in defineUiOrdering or constructor):
```cpp
void MyClass::defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering)
{
    m_azimuthAngle.uiCapability()->setAttributeInt("minimum", 0);
    m_azimuthAngle.uiCapability()->setAttributeInt("maximum", 360);
    m_azimuthAngle.uiCapability()->setAttributeInt("sliderTickCount", 360);

    uiOrdering.add(&m_azimuthAngle);
}
```

### 2. Dynamic Slider Attributes

**Example**: Measured depth slider based on well path

**Before** (defineEditorAttribute):
```cpp
void MyClass::defineEditorAttribute(const caf::PdmFieldHandle* field,
                                    QString uiConfigName,
                                    caf::PdmUiEditorAttribute* attribute)
{
    if (field == &m_measuredDepth)
    {
        auto* attr = dynamic_cast<caf::PdmUiDoubleSliderEditorAttribute*>(attribute);
        if (attr && wellPath())
        {
            attr->m_minimum = wellPath()->startMD();
            attr->m_maximum = wellPath()->endMD();
        }
    }
}
```

**After** (in defineUiOrdering):
```cpp
void MyClass::defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering)
{
    auto wellPath = firstAncestorOrThisOfType<RimWellPath>();
    if (wellPath)
    {
        m_measuredDepth.uiCapability()->setAttributeDouble("minimum", wellPath->startMD());
        m_measuredDepth.uiCapability()->setAttributeDouble("maximum", wellPath->endMD());
    }

    uiOrdering.add(&m_measuredDepth);
}
```

### 3. Dynamic Button Text

**Example**: Toggle button with conditional text

**Before** (defineEditorAttribute):
```cpp
void MyClass::defineEditorAttribute(const caf::PdmFieldHandle* field,
                                    QString uiConfigName,
                                    caf::PdmUiEditorAttribute* attribute)
{
    if (field == &m_show3DManipulator)
    {
        auto* attr = dynamic_cast<caf::PdmUiPushButtonEditorAttribute*>(attribute);
        if (attr)
        {
            if (m_show3DManipulator)
                attr->m_buttonText = "Hide 3D manipulator";
            else
                attr->m_buttonText = "Show 3D manipulator";
        }
    }
}
```

**After** (in defineUiOrdering):
```cpp
void MyClass::defineUiOrdering(QString uiConfigName, caf::PdmUiOrdering& uiOrdering)
{
    if (m_show3DManipulator)
        m_show3DManipulator.uiCapability()->setAttributeString("buttonText", "Hide 3D manipulator");
    else
        m_show3DManipulator.uiCapability()->setAttributeString("buttonText", "Show 3D manipulator");

    uiOrdering.add(&m_show3DManipulator);
}
```

### 4. Removing defineEditorAttribute

After migrating all attributes:

1. Remove the method implementation from `.cpp`:
```cpp
// DELETE THIS:
void MyClass::defineEditorAttribute(...) { ... }
```

2. Remove the method declaration from `.h`:
```cpp
// DELETE THIS:
void defineEditorAttribute(const caf::PdmFieldHandle* field,
                          QString uiConfigName,
                          caf::PdmUiEditorAttribute* attribute) override;
```

## Attribute Name Mapping

| Old (defineEditorAttribute) | New (setAttribute*) |
|----------------------------|---------------------|
| `attr->m_minimum`          | `"minimum"`         |
| `attr->m_maximum`          | `"maximum"`         |
| `attr->m_sliderTickCount`  | `"sliderTickCount"` |
| `attr->m_buttonText`       | `"buttonText"`      |
| `attr->placeholderText`    | `"placeholderText"` |
| `attr->maximumWidth`       | `"maximumWidth"`    |

## Setter Methods

| Method | Use For |
|--------|---------|
| `setAttributeInt(name, value)` | Integer values |
| `setAttributeDouble(name, value)` | Double/float values |
| `setAttributeString(name, value)` | String values |
| `setAttributeBool(name, value)` | Boolean values |

## When to Keep defineEditorAttribute

Keep `defineEditorAttribute()` only for:
- Complex attributes that cannot be stored in QVariant:
  - `QFont` objects
  - Event handlers (`pickEventHandler`)
  - Field pointers (`currentIndexFieldHandle`)
  - Validators
- Arrays/vectors (`columnWidths`, custom arrays)

For 95% of cases, use the map-based system.

## Workflow

1. **Pick a file** from the categorized list (start with simple sliders)
2. **Read** the defineEditorAttribute implementation
3. **Identify** the pattern (static/dynamic slider, button text, etc.)
4. **Add** attribute calls to defineUiOrdering() or constructor
5. **Remove** defineEditorAttribute method from .h and .cpp
6. **Build** and verify compilation
7. **Test** the UI to ensure attributes work correctly
8. **Commit** in small batches (5-10 files)

## Tips

- Start with simple files to build confidence
- Do files in batches of 5-10, commit after each batch
- Test the UI after each migration
- If unsure, look at already-migrated files for examples
- For complex cases, it's OK to keep defineEditorAttribute

## Finding Supported Attributes

Each editor validates attributes at runtime. If you set an unsupported attribute, you'll get a warning:

```
PdmUiLineEditor: Unsupported attribute 'maxWidth' set on field.
Supported attributes are: maximumWidth, selectAllOnFocusEvent, placeholderText, ...
```

To find supported attributes:
1. Check the editor's header file (e.g., `cafPdmUiLineEditor.h`)
2. Check validation code in the editor's .cpp file
3. Trigger a warning by setting a dummy attribute and reading the error message

## Example Commits

See these commits for examples:
- Batch 1: RimPerforationInterval, RimPolygon
- Batch 2: RicCellRangeUi, RimWellPathValve
- Batch 3: RimWellPathTieIn, RimBoxIntersection
