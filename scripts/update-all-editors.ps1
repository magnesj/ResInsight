# Master script to update all PdmUi editors with Keys struct pattern
# Processes editors one at a time with individual commits

$ErrorActionPreference = "Stop"

# List of editors to update (excluding already done: PdmUiListEditor, PdmUiLineEditor)
$editorsToUpdate = @(
    'cafPdmUiLabelEditor',
    'cafPdmUiTableViewEditor',
    'cafPdmUiComboBoxEditor',
    'cafPdmUiSliderEditor',
    'cafPdmUiColorEditor',
    'cafPdmUiDateEditor',
    'cafPdmUiDoubleSliderEditor',
    'cafPdmUiDoubleValueEditor',
    'cafPdmUiFilePathEditor',
    'cafPdmUiPickableLineEditor',
    'cafPdmUiPushButtonEditor',
    'cafPdmUiTextEditor',
    'cafPdmUiTimeEditor',
    'cafPdmUiToolButtonEditor',
    'cafPdmUiToolButtonCallbackEditor',
    'cafPdmUiTreeSelectionEditor'
)

Write-Host "=== Batch Update PdmUi Editors ===" -ForegroundColor Cyan
Write-Host "Will process $($editorsToUpdate.Count) editors" -ForegroundColor White
Write-Host ""

$successCount = 0
$skippedCount = 0
$failedCount = 0
$failedEditors = @()

foreach ($editor in $editorsToUpdate) {
    Write-Host "[$($successCount + $skippedCount + $failedCount + 1)/$($editorsToUpdate.Count)] Processing $editor..." -ForegroundColor White
    
    try {
        & "$PSScriptRoot\update-single-editor.ps1" -EditorName $editor -Commit
        
        if ($LASTEXITCODE -eq 0) {
            $successCount++
            Write-Host "  ? Success" -ForegroundColor Green
        } elseif ($LASTEXITCODE -eq 2) {
            $skippedCount++
            Write-Host "  ? Skipped (no attributes or already done)" -ForegroundColor Yellow
        } else {
            $failedCount++
            $failedEditors += $editor
            Write-Host "  ? Failed" -ForegroundColor Red
        }
    } catch {
        $failedCount++
        $failedEditors += $editor
        Write-Host "  ? Error: $_" -ForegroundColor Red
    }
    
    Write-Host ""
}

# Summary
Write-Host "=== Summary ===" -ForegroundColor Cyan
Write-Host "Total:   $($editorsToUpdate.Count)" -ForegroundColor White
Write-Host "Success: $successCount" -ForegroundColor Green
Write-Host "Skipped: $skippedCount" -ForegroundColor Yellow
Write-Host "Failed:  $failedCount" -ForegroundColor $(if ($failedCount -gt 0) { 'Red' } else { 'Green' })

if ($failedEditors.Count -gt 0) {
    Write-Host "`nFailed editors:" -ForegroundColor Red
    foreach ($editor in $failedEditors) {
        Write-Host "  - $editor" -ForegroundColor Red
    }
}

Write-Host "`n=== All Done ===" -ForegroundColor Cyan
