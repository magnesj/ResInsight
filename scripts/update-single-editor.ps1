# Script to update a single PdmUi editor with Keys struct pattern
param(
    [Parameter(Mandatory=$true)]
    [string]$EditorName,
    
    [switch]$Commit = $false
)

$ErrorActionPreference = "Stop"
$projectRoot = "M:\gitroot\ResInsight"
$uiDir = Join-Path $projectRoot "Fwk\AppFwk\cafUserInterface"

# Function to convert attribute name to constant name
function Convert-ToConstName {
    param([string]$attrName)
    
    $result = ""
    for ($i = 0; $i -lt $attrName.Length; $i++) {
        $char = $attrName[$i]
        if ([char]::IsUpper($char) -and $i -gt 0) {
            $result += "_"
        }
        $result += [char]::ToUpper($char)
    }
    return $result
}

# Extract attributes from CPP file
function Get-EditorAttributes {
    param([string]$cppFile)
    
    if (!(Test-Path $cppFile)) {
        Write-Host "CPP file not found: $cppFile" -ForegroundColor Red
        return @()
    }
    
    $content = Get-Content $cppFile -Raw
    $pattern = 'getAttribute<[^>]+>\s*\(\s*"([^"]+)"'
    $matches = [regex]::Matches($content, $pattern)
    
    $attrs = @{}
    foreach ($match in $matches) {
        $attrName = $match.Groups[1].Value
        if (!$attrs.ContainsKey($attrName)) {
            $attrs[$attrName] = $true
        }
    }
    
    return ($attrs.Keys | Sort-Object)
}

# Main execution
Write-Host "=== Updating $EditorName ===" -ForegroundColor Cyan

$headerFile = Join-Path $uiDir "$EditorName.h"
$cppFile = Join-Path $uiDir "$EditorName.cpp"

if (!(Test-Path $cppFile)) {
    Write-Host "Editor not found: $cppFile" -ForegroundColor Red
    exit 1
}

# Get attributes
$attributes = Get-EditorAttributes -cppFile $cppFile

if ($attributes.Count -eq 0) {
    Write-Host "No attributes found in $EditorName" -ForegroundColor Yellow
    exit 0
}

Write-Host "Found $($attributes.Count) attributes:" -ForegroundColor Green
foreach ($attr in $attributes) {
    Write-Host "  - $attr" -ForegroundColor Gray
}

# Check if already updated
$headerContent = Get-Content $headerFile -Raw
if ($headerContent -match 'struct Keys\s*\{') {
    Write-Host "Keys struct already exists in $EditorName" -ForegroundColor Yellow
    exit 0
}

# Generate Keys struct
$keysStruct = @"

    // Attribute key constants for compile-time safety and discoverability
    struct Keys
    {
"@

foreach ($attr in $attributes) {
    $constName = Convert-ToConstName $attr
    $keysStruct += "`n        static inline const QString $constName = QStringLiteral(`"$attr`");"
}

$keysStruct += @"

    };

    // Set of all supported attributes for validation
    inline static const std::set<QString> SUPPORTED_ATTRIBUTES = {
"@

$keysList = @()
foreach ($attr in $attributes) {
    $constName = Convert-ToConstName $attr
    $keysList += "        Keys::$constName"
}
$keysStruct += "`n" + ($keysList -join ",`n") + "`n    };"

# Update header file
$pattern = "(class\s+$EditorName\s*:.*?\{.*?public:)"
if ($headerContent -match $pattern) {
    $insertPoint = $matches[1]
    $newContent = $headerContent -replace [regex]::Escape($insertPoint), ($insertPoint + $keysStruct)
    Set-Content -Path $headerFile -Value $newContent -NoNewline
    Write-Host "? Updated header file" -ForegroundColor Green
} else {
    Write-Host "? Could not find insertion point in header" -ForegroundColor Red
    exit 1
}

# Update CPP file
$cppContent = Get-Content $cppFile -Raw

# Replace getAttribute calls
foreach ($attr in $attributes) {
    $constName = Convert-ToConstName $attr
    $cppContent = $cppContent -replace "getAttribute<([^>]+)>\s*\(\s*`"$attr`"", "getAttribute<`$1>( Keys::$constName"
}

# Remove old supportedAttributes declaration
$cppContent = $cppContent -replace 'static const std::set<QString>\s+supportedAttributes\s*=\s*\{[^\}]+\};', ''

# Replace supportedAttributes references with SUPPORTED_ATTRIBUTES
$cppContent = $cppContent -replace 'supportedAttributes\.find', 'SUPPORTED_ATTRIBUTES.find'
$cppContent = $cppContent -replace 'supportedAttributes\.end\(\)', 'SUPPORTED_ATTRIBUTES.end()'

# Update warning message
$warningPattern = 'CAF_PDM_LOG_WARNING\(\s*QString\(\s*"[^"]*:\s*Unsupported attribute[^"]*"\s*\)\s*\.arg\(\s*key\s*\)\s*\);'
$newWarning = @"
CAF_PDM_LOG_WARNING(
                            QString( "$EditorName`: Unsupported attribute '%1' set on field. Supported "
                                     "attributes are: %2" )
                                .arg( key )
                                .arg( QStringList( SUPPORTED_ATTRIBUTES.begin(), SUPPORTED_ATTRIBUTES.end() ).join( ", " ) ) );
"@
$cppContent = $cppContent -replace $warningPattern, $newWarning

Set-Content -Path $cppFile -Value $cppContent -NoNewline
Write-Host "? Updated CPP file" -ForegroundColor Green

# Build to verify
Write-Host "`nBuilding to verify changes..." -ForegroundColor Cyan
Push-Location $projectRoot
try {
    $buildResult = cmake --build ThirdParty/vcpkg/buildtrees/versioning_/versions --target cafUserInterface 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Host "? Build successful" -ForegroundColor Green
    } else {
        Write-Host "? Build failed" -ForegroundColor Red
        Write-Host $buildResult
        exit 1
    }
} finally {
    Pop-Location
}

# Commit if requested
if ($Commit) {
    Write-Host "`nCommitting changes..." -ForegroundColor Cyan
    Push-Location $projectRoot
    try {
        git add $headerFile $cppFile
        $commitMsg = @"
Refactor($EditorName): Add Keys struct for attribute names

- Add Keys struct with QString constants using QStringLiteral
- Add SUPPORTED_ATTRIBUTES set for validation
- Replace string literals with Keys:: constants in getAttribute calls
- Update validation to dynamically list supported attributes

Attributes: $($attributes -join ', ')
"@
        git commit -m $commitMsg
        Write-Host "? Changes committed" -ForegroundColor Green
    } catch {
        Write-Host "? Commit failed: $_" -ForegroundColor Red
        exit 1
    } finally {
        Pop-Location
    }
}

Write-Host "`n=== Done updating $EditorName ===" -ForegroundColor Cyan
