$ErrorActionPreference = "Stop"

function Get-ProjectRoot {
    $scriptPath = $PSScriptRoot
    return Split-Path $scriptPath -Parent
}

$projectRoot = Get-ProjectRoot
Set-Location $projectRoot

Write-Host "Running pre-commit checks..." -ForegroundColor Cyan

# Check for required tools
$clangFormat = Get-Command "clang-format" -ErrorAction SilentlyContinue
if (-not $clangFormat) {
    Write-Host "Error: clang-format not found in PATH." -ForegroundColor Red
    exit 1
}

$clangTidy = Get-Command "clang-tidy" -ErrorAction SilentlyContinue
if (-not $clangTidy) {
    Write-Host "Warning: clang-tidy not found in PATH. Skipping static analysis." -ForegroundColor Yellow
}

# 1. Get Staged Files
Write-Host "Checking for staged files..." -ForegroundColor Cyan
$stagedFiles = @(git diff --cached --name-only --diff-filter=ACMR)

if (-not $stagedFiles) {
    Write-Host "No staged files found." -ForegroundColor Green
    exit 0
}

# Filter for relevant extensions
$formatExtensions = @(".cpp", ".h", ".hpp", ".ixx", ".cppm", ".c", ".cc")
$tidyExtensions = @(".cpp", ".ixx", ".cppm", ".c", ".cc")

$filesToFormat = @()
$filesToTidy = @()

foreach ($file in $stagedFiles) {
    if ([string]::IsNullOrWhiteSpace($file)) { continue }
    $ext = [System.IO.Path]::GetExtension($file)
    
    if (Test-Path $file) {
        if ($formatExtensions -contains $ext) {
            $filesToFormat += $file
        }
        if ($tidyExtensions -contains $ext) {
            $filesToTidy += $file
        }
    }
}

if ($filesToFormat.Count -eq 0) {
    Write-Host "No C++ source files to check." -ForegroundColor Green
    exit 0
}

Write-Host "Found $($filesToFormat.Count) files to process." -ForegroundColor Cyan

# 2. Format and Re-stage
Write-Host "Running code formatting..." -ForegroundColor Cyan
$formatErrors = $false

foreach ($file in $filesToFormat) {
    Write-Host "Formatting: $file"
    & $clangFormat -i -style=file $file
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Failed to format: $file" -ForegroundColor Red
        $formatErrors = $true
    } else {
        # Re-stage the file to include formatting changes
        git add $file
    }
}

if ($formatErrors) {
    Write-Host "Code formatting encountered errors." -ForegroundColor Red
    exit 1
}

# 3. Clang-Tidy
if ($clangTidy -and $filesToTidy.Count -gt 0) {
    # Find build directory with compile_commands.json
    $buildDirPattern = Join-Path $projectRoot "build\*"
    $buildDirs = Get-Item $buildDirPattern | Where-Object { Test-Path (Join-Path $_.FullName "compile_commands.json") }
    
    if ($buildDirs) {
        # Use the first one found (prefer Debug if multiple)
        $buildDir = $buildDirs[0].FullName
        Write-Host "Running static analysis using build directory: $buildDir" -ForegroundColor Cyan
        
        # Run clang-tidy on the list of implementation files
        & $clangTidy -p $buildDir $filesToTidy
        
        if ($LASTEXITCODE -ne 0) {
             Write-Host "Static analysis found issues. Please fix them before committing." -ForegroundColor Red
             exit 1
        }
    } else {
        Write-Host "Skipping static analysis (compile_commands.json not found in build directories)." -ForegroundColor Yellow
    }
} elseif ($filesToTidy.Count -eq 0) {
     Write-Host "No implementation files to static analyze." -ForegroundColor Green
}

Write-Host "Pre-commit checks passed." -ForegroundColor Green
exit 0
