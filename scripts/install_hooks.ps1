$ErrorActionPreference = "Stop"
$scriptPath = $PSScriptRoot
$projectRoot = Split-Path $scriptPath -Parent
$hooksDir = Join-Path $projectRoot ".git\hooks"
$sourceHook = Join-Path $scriptPath "git_hooks\pre-commit"

if (-not (Test-Path $hooksDir)) {
    Write-Host "Error: .git directory not found. Is this a git repository?" -ForegroundColor Red
    exit 1
}

Copy-Item -Path $sourceHook -Destination (Join-Path $hooksDir "pre-commit") -Force
Write-Host "Git pre-commit hook installed successfully." -ForegroundColor Green
