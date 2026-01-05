# Simple batch update script for PdmUi editors
# Updates all getAttribute calls to use Keys:: constants

$editors = @{
    'cafPdmUiLabelEditor' = @('useWordWrap', 'useSingleWidgetInsteadOfLabelAndEditorWidget', 'linkText', 'linkActivatedCallback')
    'cafPdmUiTableViewEditor' = @('tableSelectionLevel', 'rowSelectionLevel', 'enableHeaderText', 'minimumHeight', 'heightHint', 'alwaysEnforceResizePolicy', 'resizePolicy', 'columnWidths', 'baseColor', 'enableDropTarget')
    'cafPdmUiComboBoxEditor' = @('iconSize', 'nextColumnIndex', 'optionVisibility')
    'cafPdmUiSliderEditor' = @('sliderTickCount', 'delaySliderUpdateUntilReleased', 'hideSpinBox')
}

function Convert-ToConstName {
    param([string]$attrName)
    $result = ""
    for ($i = 0; $i < $attrName.Length; $i++) {
        $char = $attrName[$i]
        if ([char]::IsUpper($char) -and $i -gt 0) {
            $result += "_"
        }
        $result += [char]::ToUpper($char)
    }
    return $result
}

foreach ($editorEntry in $editors.GetEnumerator()) {
    $editor = $editorEntry.Key
    $attrs = $editorEntry.Value
    
    Write-Host "Processing $editor..." -ForegroundColor Cyan
    
    # Generate and display the Keys struct
    Write-Host "  Keys struct:" -ForegroundColor Yellow
    foreach ($attr in $attrs) {
        $constName = Convert-ToConstName $attr
        Write-Host "    static inline const QString $constName = QStringLiteral(`"$attr`");"
    }
    Write-Host ""
}
