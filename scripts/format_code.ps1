<#
.SYNOPSIS
    Formats C++ source code using clang-format.

.DESCRIPTION
    Scans the project directories (src, facefusionCpp, tests) for C++ source files
    and applies formatting using the .clang-format configuration file.
#>

$ErrorActionPreference = "Stop"

function Get-ProjectRoot {
    $scriptPath = $PSScriptRoot
    return Split-Path $scriptPath -Parent
}

$projectRoot = Get-ProjectRoot
$clangFormat = Get-Command "clang-format" -ErrorAction SilentlyContinue

if (-not $clangFormat) {
    Write-Host "Error: clang-format executable not found in PATH." -ForegroundColor Red
    Write-Host "Please install LLVM/Clang and ensure bin directory is in your PATH."
    exit 1
}

Write-Host "Using clang-format: $($clangFormat.Source)" -ForegroundColor Cyan
Write-Host "Project Root: $projectRoot" -ForegroundColor Cyan

# Directories to scan
$dirs = @("src", "facefusionCpp", "tests")
# Extensions to match
$extensions = @("*.cpp", "*.h", "*.hpp", "*.ixx", "*.cppm", "*.c", "*.cc")

$filesToFormat = @()

foreach ($dir in $dirs) {
    $path = Join-Path $projectRoot $dir
    if (Test-Path $path) {
        Write-Host "Scanning directory: $dir" -ForegroundColor Green
        foreach ($ext in $extensions) {
            $files = Get-ChildItem -Path $path -Recurse -Filter $ext
            $filesToFormat += $files
        }
    } else {
        Write-Host "Directory not found: $dir" -ForegroundColor Yellow
    }
}

if ($filesToFormat.Count -eq 0) {
    Write-Host "No files found to format." -ForegroundColor Yellow
    exit 0
}

Write-Host "Found $($filesToFormat.Count) files. Starting formatting..." -ForegroundColor Cyan

foreach ($file in $filesToFormat) {
    Write-Host "Formatting: $($file.FullName)"
    & $clangFormat -i -style=file $($file.FullName)
    if ($LASTEXITCODE -ne 0) {
        Write-Host "Error formatting file: $($file.FullName)" -ForegroundColor Red
    }
}

Write-Host "Formatting complete." -ForegroundColor Green
