# PowerShell script to add Keys struct and SUPPORTED_ATTRIBUTES to all PdmUi editors
# This script automates the refactoring of string-based attributes to named constants

param(
    [string]$EditorName = "",
    [switch]$DryRun = $false
)

$projectRoot = "M:\gitroot\ResInsight"
$userInterfaceDir = Join-Path $projectRoot "Fwk\AppFwk\cafUserInterface"

# List of editors that use getAttribute
$editors = @(
    'cafPdmUiColorEditor',
    'cafPdmUiComboBoxEditor',
    'cafPdmUiDateEditor',
    'cafPdmUiDoubleSliderEditor',
    'cafPdmUiDoubleValueEditor',
    'cafPdmUiFilePathEditor',
    'cafPdmUiLabelEditor',
    'cafPdmUiLineEditor',
    'cafPdmUiPickableLineEditor',
    'cafPdmUiPushButtonEditor',
    'cafPdmUiSliderEditor',
    'cafPdmUiTableViewEditor',
    'cafPdmUiTextEditor',
    'cafPdmUiTimeEditor',
    'cafPdmUiToolButtonCallbackEditor',
    'cafPdmUiToolButtonEditor',
    'cafPdmUiTreeSelectionEditor'
)

# Skip PdmUiListEditor as it's already done
$editors = $editors | Where-Object { $_ -ne 'cafPdmUiListEditor' }

function Get-AttributesFromCpp {
    param([string]$cppFile)
    
    if (!(Test-Path $cppFile)) {
        return @()
    }
    
    $content = Get-Content $cppFile -Raw
    $pattern = 'getAttribute<[^>]+>\s*\(\s*"([^"]+)"'
    $matches = [regex]::Matches($content, $pattern)
    
    $attributes = @()
    foreach ($match in $matches) {
        $attrName = $match.Groups[1].Value
        if ($attributes -notcontains $attrName) {
            $attributes += $attrName
        }
    }
    
    return $attributes | Sort-Object
}

function Convert-ToCamelCase {
    param([string]$snakeCase)
    
    $parts = $snakeCase -split '[A-Z]' | Where-Object { $_ }
    $result = ""
    foreach ($part in $snakeCase.ToCharArray()) {
        if ([char]::IsUpper($part)) {
            $result += "_" + $part
        } else {
            $result += [char]::ToUpper($part)
        }
    }
    return $result.TrimStart('_')
}

function Update-EditorHeader {
    param(
        [string]$editorName,
        [string[]]$attributes
    )
    
    $headerFile = Join-Path $userInterfaceDir "$editorName.h"
    if (!(Test-Path $headerFile)) {
        Write-Host "Header not found: $headerFile" -ForegroundColor Yellow
        return $false
    }
    
    $content = Get-Content $headerFile -Raw
    
    # Check if Keys struct already exists
    if ($content -match 'struct Keys\s*\{') {
        Write-Host "  Keys struct already exists in $editorName.h" -ForegroundColor Cyan
        return $true
    }
    
    # Generate Keys struct
    $keysStruct = @"
    // Attribute key constants for compile-time safety and discoverability
    struct Keys
    {
"@
    
    foreach ($attr in $attributes) {
        $constName = Convert-ToCamelCase $attr
        $keysStruct += "`n        static inline const QString $constName = QStringLiteral(`"$attr`");"
    }
    
    $keysStruct += @"

    };

    // Set of all supported attributes for validation
    inline static const std::set<QString> SUPPORTED_ATTRIBUTES = {
"@
    
    $keysList = ($attributes | ForEach-Object { "        Keys::$(Convert-ToCamelCase $_)" }) -join ",`n"
    $keysStruct += "`n$keysList`n    };"
    
    # Find insertion point (after class declaration, before first public method)
    $pattern = '(class\s+' + $editorName + '\s*:.*?{[^}]*?public:)'
    if ($content -match $pattern) {
        $insertPoint = $matches[1]
        $replacement = $insertPoint + "`n`n" + $keysStruct
        $content = $content -replace [regex]::Escape($insertPoint), $replacement
        
        if (!$DryRun) {
            Set-Content -Path $headerFile -Value $content -NoNewline
        }
        
        Write-Host "  Updated header: $editorName.h" -ForegroundColor Green
        return $true
    }
    
    Write-Host "  Could not find insertion point in $editorName.h" -ForegroundColor Yellow
    return $false
}

function Update-EditorCpp {
    param(
        [string]$editorName,
        [string[]]$attributes
    )
    
    $cppFile = Join-Path $userInterfaceDir "$editorName.cpp"
    if (!(Test-Path $cppFile)) {
        Write-Host "CPP not found: $cppFile" -ForegroundColor Yellow
        return $false
    }
    
    $content = Get-Content $cppFile -Raw
    
    # Replace string literals with Keys:: constants
    foreach ($attr in $attributes) {
        $constName = Convert-ToCamelCase $attr
        # Replace getAttribute calls
        $content = $content -replace "getAttribute<([^>]+)>\s*\(\s*`"$attr`"", "getAttribute<`$1>( Keys::$constName"
    }
    
    # Replace static supportedAttributes with SUPPORTED_ATTRIBUTES
    $content = $content -replace 'static const std::set<QString>\s+supportedAttributes\s*=\s*\{[^}]+\};', ''
    $content = $content -replace 'supportedAttributes\.find', 'SUPPORTED_ATTRIBUTES.find'
    
    # Update warning message to use SUPPORTED_ATTRIBUTES
    $pattern = 'CAF_PDM_LOG_WARNING\(\s*QString\([^)]+\)\s*\.arg\(\s*key\s*\)\s*\);'
    $replacement = @'
CAF_PDM_LOG_WARNING( QString( "$editorName: Unsupported attribute '%1' set on field. Supported attributes are: %2" )
                                         .arg( key )
                                         .arg( QStringList( SUPPORTED_ATTRIBUTES.begin(), SUPPORTED_ATTRIBUTES.end() ).join( ", " ) ) );
'@ -replace '\$editorName', $editorName
    
    $content = $content -replace $pattern, $replacement
    
    if (!$DryRun) {
        Set-Content -Path $cppFile -Value $content -NoNewline
    }
    
    Write-Host "  Updated CPP: $editorName.cpp" -ForegroundColor Green
    return $true
}

# Main execution
Write-Host "=== PdmUi Editor Attribute Refactoring ===" -ForegroundColor Cyan
Write-Host "Mode: $(if ($DryRun) { 'DRY RUN' } else { 'LIVE' })" -ForegroundColor $(if ($DryRun) { 'Yellow' } else { 'Red' })
Write-Host ""

$editorsToProcess = if ($EditorName) { @($EditorName) } else { $editors }

foreach ($editor in $editorsToProcess) {
    Write-Host "Processing: $editor" -ForegroundColor White
    
    $cppFile = Join-Path $userInterfaceDir "$editor.cpp"
    $attributes = Get-AttributesFromCpp $cppFile
    
    if ($attributes.Count -eq 0) {
        Write-Host "  No attributes found, skipping" -ForegroundColor Gray
        continue
    }
    
    Write-Host "  Found $($attributes.Count) attributes: $($attributes -join ', ')" -ForegroundColor Gray
    
    $headerSuccess = Update-EditorHeader -editorName $editor -attributes $attributes
    $cppSuccess = Update-EditorCpp -editorName $editor -attributes $attributes
    
    if ($headerSuccess -and $cppSuccess) {
        Write-Host "  ? Successfully updated $editor" -ForegroundColor Green
    } else {
        Write-Host "  ? Failed to update $editor" -ForegroundColor Red
    }
    
    Write-Host ""
}

Write-Host "=== Done ===" -ForegroundColor Cyan
