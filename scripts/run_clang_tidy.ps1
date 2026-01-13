<#
.SYNOPSIS
    Runs clang-tidy static analysis.

.DESCRIPTION
    Runs clang-tidy on the project source files. Requires compile_commands.json to be generated.
#>

$ErrorActionPreference = "Stop"

function Get-ProjectRoot {
    $scriptPath = $PSScriptRoot
    return Split-Path $scriptPath -Parent
}

$projectRoot = Get-ProjectRoot
$clangTidy = Get-Command "clang-tidy" -ErrorAction SilentlyContinue

if (-not $clangTidy) {
    Write-Host "Error: clang-tidy executable not found in PATH." -ForegroundColor Red
    Write-Host "Please install LLVM/Clang and ensure bin directory is in your PATH."
    exit 1
}

# Find build directory with compile_commands.json
# Usually in build/msvc-x64-debug or similar
$buildDirPattern = Join-Path $projectRoot "build\*"
$buildDirs = Get-Item $buildDirPattern | Where-Object { Test-Path (Join-Path $_.FullName "compile_commands.json") }

if (-not $buildDirs) {
    Write-Host "Error: compile_commands.json not found in any build directory." -ForegroundColor Red
    Write-Host "Please run configuration first (e.g., .\build.ps1 -Action configure)."
    exit 1
}

# Use the first one found (prefer Debug if multiple?)
$buildDir = $buildDirs[0].FullName
Write-Host "Using build directory: $buildDir" -ForegroundColor Cyan
Write-Host "Using clang-tidy: $($clangTidy.Source)" -ForegroundColor Cyan

# Directories to scan for files to check
$dirs = @("src")
$extensions = @("*.cpp", "*.ixx", "*.cppm", "*.cc", "*.c") # clang-tidy usually runs on implementation files

$filesToCheck = @(foreach ($dir in $dirs) {
    $path = Join-Path $projectRoot $dir
    if (Test-Path $path) {
        Write-Host "Scanning directory: $dir" -ForegroundColor Green
        foreach ($ext in $extensions) {
            @(Get-ChildItem -Path $path -Recurse -Filter $ext)
        }
    }
})

if ($filesToCheck.Count -eq 0) {
    Write-Host "No files found to check." -ForegroundColor Yellow
    exit 0
}

Write-Host "Found $($filesToCheck.Count) files. Collecting for batch analysis..." -ForegroundColor Cyan

$filePaths = $filesToCheck | ForEach-Object { $_.FullName }

# Define batch size (e.g., 50 files per call to avoid command line limits)
# Windows command line limit is 32k characters, 50 paths should be safe.
$batchSize = 50 

for ($i = 0; $i -lt $filePaths.Count; $i += $batchSize) {
    $batch = $filePaths | Select-Object -Skip $i -First $batchSize
    
    Write-Host "Checking batch ($($i + 1) to $([Math]::Min($i + $batchSize, $filePaths.Count)))..." -ForegroundColor Cyan
    
    # Execute clang-tidy with the batch of files
    # -p specifies the build path for compile_commands.json
    & $clangTidy -p $buildDir $batch
    
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Warnings or errors found in batch." -ForegroundColor Yellow
    }
}

Write-Host "Analysis complete." -ForegroundColor Green
