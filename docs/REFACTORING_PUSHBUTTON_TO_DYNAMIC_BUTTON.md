# Refactoring Guide: PushButtonEditor Fields ? Dynamic Buttons

## Overview
Replace static push button fields with dynamic buttons using `PdmUiOrdering::addNewButton()`.

## Benefits
- **Less code**: No field declaration, initialization, or state management
- **No XML serialization**: Buttons don't belong in project files
- **Cleaner logic**: No need for `fieldChangedByUi()` handling
- **No attribute setup**: Button text is defined inline
- **No state reset**: No need to set button field back to `false`

## Refactoring Steps

### 1. Remove Field Declaration (Header File)
**Before:**
```cpp
class RiaPreferences : public caf::PdmObject
{
private:
    caf::PdmField<bool> m_deleteOsduToken;
    caf::PdmField<bool> m_deleteSumoToken;
};
```

**After:**
```cpp
class RiaPreferences : public caf::PdmObject
{
private:
    // Fields removed - buttons are now dynamic
};
```

### 2. Remove Field Initialization (Constructor)
**Before:**
```cpp
RiaPreferences::RiaPreferences()
{
    CAF_PDM_InitField( &m_deleteOsduToken, "deleteOsduToken", false, "" );
    caf::PdmUiPushButtonEditor::configureEditorLabelHidden( &m_deleteOsduToken );
    m_deleteOsduToken.xmlCapability()->disableIO();
    
    CAF_PDM_InitField( &m_deleteSumoToken, "deleteSumoToken", false, "" );
    caf::PdmUiPushButtonEditor::configureEditorLabelHidden( &m_deleteSumoToken );
    m_deleteSumoToken.xmlCapability()->disableIO();
}
```

**After:**
```cpp
RiaPreferences::RiaPreferences()
{
    // Field initialization removed - buttons are dynamic
}
```

### 3. Remove fieldChangedByUi() Handling
**Before:**
```cpp
void RiaPreferences::fieldChangedByUi( const caf::PdmFieldHandle* changedField, 
                                       const QVariant& oldValue, 
                                       const QVariant& newValue )
{
    if ( changedField == &m_deleteOsduToken )
    {
        RicDeleteOsduTokenFeature::deleteUserToken();
        m_deleteOsduToken = false;
    }
    
    if ( changedField == &m_deleteSumoToken )
    {
        RicDeleteSumoTokenFeature::deleteUserToken();
        m_deleteSumoToken = false;
    }
}
```

**After:**
```cpp
void RiaPreferences::fieldChangedByUi( const caf::PdmFieldHandle* changedField, 
                                       const QVariant& oldValue, 
                                       const QVariant& newValue )
{
    // Button handling removed - now done in lambda
}
```

### 4. Remove defineEditorAttribute() Handling
**Before:**
```cpp
void RiaPreferences::defineEditorAttribute( const caf::PdmFieldHandle* field, 
                                           QString uiConfigName, 
                                           caf::PdmUiEditorAttribute* attribute )
{
    if ( field == &m_deleteOsduToken )
    {
        auto* attr = dynamic_cast<caf::PdmUiPushButtonEditorAttribute*>( attribute );
        if ( attr )
        {
            attr->m_buttonText = "Delete OSDU Token";
        }
    }
    
    if ( field == &m_deleteSumoToken )
    {
        auto* attr = dynamic_cast<caf::PdmUiPushButtonEditorAttribute*>( attribute );
        if ( attr )
        {
            attr->m_buttonText = "Delete SUMO Token";
        }
    }
}
```

**After:**
```cpp
void RiaPreferences::defineEditorAttribute( const caf::PdmFieldHandle* field, 
                                           QString uiConfigName, 
                                           caf::PdmUiEditorAttribute* attribute )
{
    // Button attribute handling removed - text defined inline
}
```

### 5. Replace in defineUiOrdering()
**Before:**
```cpp
void RiaPreferences::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    auto osduGroup = uiOrdering.addNewGroup( "OSDU" );
    osduGroup->add( &m_deleteOsduToken );
    
    auto sumoGroup = uiOrdering.addNewGroup( "SUMO" );
    sumoGroup->add( &m_deleteSumoToken );
}
```

**After:**
```cpp
void RiaPreferences::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    auto osduGroup = uiOrdering.addNewGroup( "OSDU" );
    osduGroup->addNewButton( "Delete OSDU Token", 
                            []() 
                            {
                                RicDeleteOsduTokenFeature::deleteUserToken();
                            } );
    
    auto sumoGroup = uiOrdering.addNewGroup( "SUMO" );
    sumoGroup->addNewButton( "Delete SUMO Token", 
                            []() 
                            {
                                RicDeleteSumoTokenFeature::deleteUserToken();
                            } );
}
```

## Advanced Patterns

### Capturing `this` for Member Access
If you need to access member variables or call member functions:

```cpp
void RiaPreferencesSummary::appendItemsToPlottingGroup( caf::PdmUiOrdering& uiOrdering )
{
    if ( m_defaultSummaryPlot() == DefaultSummaryPlotType::PLOT_TEMPLATES )
    {
        uiOrdering.add( &m_selectedDefaultTemplates );
        
        // Capture this to access member variables
        uiOrdering.addNewButton( "Select Default Templates",
                                [this]()
                                {
                                    auto selection = RicSummaryPlotTemplateTools::selectDefaultPlotTemplates( 
                                        m_selectedDefaultTemplates() );
                                    m_selectedDefaultTemplates.setValueWithFieldChanged( selection );
                                } );
    }
}
```

**Note:** When capturing `this`, ensure the method is non-const if you need to modify state.

### Layout Options
Buttons support the same layout options as fields:

```cpp
// Start on same row as previous item
uiOrdering.addNewButton( "My Button", callback, { .newRow = false } );

// Custom column span
uiOrdering.addNewButton( "Wide Button", callback, { .totalColumnSpan = 3 } );
```

### Conditional Buttons
Show buttons based on state:

```cpp
void MyClass::defineUiOrdering( QString uiConfigName, caf::PdmUiOrdering& uiOrdering )
{
    if ( someCondition )
    {
        uiOrdering.addNewButton( "Action A", []() { /* ... */ } );
    }
    else
    {
        uiOrdering.addNewButton( "Action B", []() { /* ... */ } );
    }
}
```

## Migration Checklist
- [ ] Remove field declaration from header
- [ ] Remove field initialization from constructor
- [ ] Remove `configureEditorLabelHidden()` call
- [ ] Remove `xmlCapability()->disableIO()` call
- [ ] Remove `fieldChangedByUi()` handling
- [ ] Remove `defineEditorAttribute()` handling
- [ ] Replace `uiOrdering.add(&field)` with `uiOrdering.addNewButton()`
- [ ] Move button action logic into lambda
- [ ] Remove field state reset logic
- [ ] Verify no XML references to removed field
- [ ] Test button functionality

## Common Patterns

### Simple Action Button
```cpp
uiOrdering.addNewButton( "Clear Data",
                        []()
                        {
                            DataManager::clearAll();
                        } );
```

### Dialog-based Action
```cpp
uiOrdering.addNewButton( "Import File",
                        [this]()
                        {
                            QString file = QFileDialog::getOpenFileName( /* ... */ );
                            if ( !file.isEmpty() )
                            {
                                importFile( file );
                            }
                        } );
```

### Update UI After Action
```cpp
uiOrdering.addNewButton( "Recalculate",
                        [this]()
                        {
                            performCalculation();
                            updateConnectedEditors();
                        } );
```

## Testing
After refactoring:
1. Verify button appears in UI
2. Verify button click performs correct action
3. Verify button text is correct
4. Verify no XML serialization occurs
5. Verify layout is correct
6. Test with multiple clicks

## Notes
- Dynamic buttons are created each time `defineUiOrdering()` is called
- Button state is not persisted (by design)
- No need for const-correctness concerns with `defineUiOrdering` when using lambdas that modify state
- Consider making `defineUiOrdering` non-const if buttons need to modify object state
