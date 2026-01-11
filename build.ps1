<#
.SYNOPSIS
    CMake build script for FaceFusionCpp project with automatic configuration detection.

.DESCRIPTION
    This script provides a unified interface for configuring and building the FaceFusionCpp project
    using CMake with MSVC toolchain. It automatically detects CMake installation and project root
    directory, and supports both Debug and Release configurations. The script automatically uses
    the system's maximum number of processor cores for parallel builds.

.PARAMETER Configuration
    Build configuration: Debug or Release. Default is Debug.

.PARAMETER Target
    Build target to execute. Default is "all" (builds all targets).
    You can specify a specific target name (e.g., "FaceFusionCpp").

.PARAMETER Action
    Action to perform: configure, build, or both. Default is both.

.EXAMPLE
    .\build.ps1
    Configure and build Debug configuration using system maximum threads.

.EXAMPLE
    .\build.ps1 -Configuration Release -Action build
    Build Release configuration only.

.EXAMPLE
    .\build.ps1 -Configuration Debug -Action configure
    Configure Debug configuration only.
#>

[CmdletBinding()]
param(
    [Parameter(Mandatory=$false)]
    [ValidateSet("Debug", "Release")]
    [string]$Configuration = "Debug",

    [Parameter(Mandatory=$false)]
    [ValidateSet("configure", "build", "test", "install", "package", "both")]
    [string]$Action = "both",

    [Parameter(Mandatory=$false)]
    [switch]$EnableCoverage,

    [Parameter(Mandatory=$false)]
    [switch]$EnableStaticAnalysis,

    [Parameter(Mandatory=$false)]
    [string]$Target = "all",

    [Parameter(Mandatory=$false)]
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"

$script:vsDevShellModule = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\Common7\Tools\Microsoft.VisualStudio.DevShell.dll"
$script:vsInstanceId = "ef10e2e8"
$script:cmakePath = $null
$script:projectRoot = $null
$script:presetName = "msvc-x64-" + $Configuration.ToLower()
$script:buildDir = $null
$script:target = $Target

function Write-Log {
    <#
    .SYNOPSIS
        Write colored log messages.
    #>
    param(
        [Parameter(Mandatory=$true)]
        [string]$Message,

        [Parameter(Mandatory=$false)]
        [ValidateSet("Info", "Success", "Warning", "Error")]
        [string]$Level = "Info"
    )

    $color = switch ($Level) {
        "Info"    { "Cyan" }
        "Success" { "Green" }
        "Warning" { "Yellow" }
        "Error"   { "Red" }
        default   { "White" }
    }

    Write-Host $Message -ForegroundColor $color
}

function Find-CMakeExecutable {
    <#
    .SYNOPSIS
        Find CMake executable in system PATH or common installation locations.
    #>
    Write-Log "Searching for CMake executable..." -Level Info

    $cmakePaths = @(
        "cmake.exe",
        "C:\Program Files\CMake\bin\cmake.exe",
        "C:\Program Files (x86)\CMake\bin\cmake.exe",
        "${env:ProgramFiles}\CMake\bin\cmake.exe",
        "${env:ProgramFiles(x86)}\CMake\bin\cmake.exe"
    )

    foreach ($path in $cmakePaths) {
        try {
            $resolvedPath = Get-Command $path -ErrorAction SilentlyContinue
            if ($resolvedPath) {
                $cmakePath = $resolvedPath.Source
                Write-Log "Found CMake at: $cmakePath" -Level Success
                return $cmakePath
            }
        }
        catch {
            continue
        }
    }

    Write-Log "CMake executable not found!" -Level Error
    Write-Log "Please install CMake and ensure it is in your PATH or installed at a standard location." -Level Info
    Write-Log "Download CMake from: https://cmake.org/download/" -Level Info
    throw "CMake not found"
}

function Find-ProjectRoot {
    <#
    .SYNOPSIS
        Automatically detect project root directory by searching for CMakeLists.txt.
    #>
    Write-Log "Detecting project root directory..." -Level Info

    $scriptPath = $PSScriptRoot
    $currentDir = $scriptPath
    $maxDepth = 10
    $depth = 0

    while ($depth -lt $maxDepth) {
        $cmakeListsPath = Join-Path $currentDir "CMakeLists.txt"

        if (Test-Path $cmakeListsPath) {
            $projectRoot = $currentDir
            Write-Log "Found project root at: $projectRoot" -Level Success
            return $projectRoot
        }

        $parentDir = Split-Path $currentDir -Parent
        if (-not $parentDir -or $parentDir -eq $currentDir) {
            break
        }

        $currentDir = $parentDir
        $depth++
    }

    Write-Log "Could not automatically detect project root directory!" -Level Error
    Write-Log "Please ensure CMakeLists.txt exists in the project root or its parent directories." -Level Info
    Write-Log "Alternatively, you can manually set the project root path in the script." -Level Info
    throw "Project root not found"
}

function Test-FileAccess {
    <#
    .SYNOPSIS
        Test if a file or directory is accessible.
    #>
    param(
        [Parameter(Mandatory=$true)]
        [string]$Path,

        [Parameter(Mandatory=$false)]
        [string]$PathType = "Any"
    )

    if (-not (Test-Path $Path -PathType $PathType)) {
        Write-Log "Path not found or inaccessible: $Path" -Level Error
        return $false
    }

    try {
        $item = Get-Item $Path -Force -ErrorAction Stop

        if ($PathType -eq "Leaf" -and $item.PSIsContainer) {
            Write-Log "Expected a file but found a directory: $Path" -Level Error
            return $false
        }

        if ($PathType -eq "Container" -and -not $item.PSIsContainer) {
            Write-Log "Expected a directory but found a file: $Path" -Level Error
            return $false
        }

        return $true
    }
    catch {
        Write-Log "Cannot access path: $Path" -Level Error
        Write-Log "Error: $_" -Level Error
        return $false
    }
}

function Initialize-Environment {
    <#
    .SYNOPSIS
        Initialize build environment by detecting and validating required tools and paths.
    #>
    Write-Log "`n=== Initializing Build Environment ===" -Level Info

    try {
        $script:cmakePath = Find-CMakeExecutable
        $script:projectRoot = Find-ProjectRoot
        $script:buildDir = Join-Path $script:projectRoot "build\$script:presetName"

        $script:maxThreads = [System.Environment]::ProcessorCount

        Write-Log "Configuration: $Configuration" -Level Info
        Write-Log "Preset: $script:presetName" -Level Info
        Write-Log "Build Directory: $script:buildDir" -Level Info
        Write-Log "Target: $script:target" -Level Info
        Write-Log "Parallel Jobs: $script:maxThreads (system maximum)" -Level Info

        Write-Log "`nEnvironment initialization completed successfully!" -Level Success
    }
    catch {
        Write-Log "`nEnvironment initialization failed!" -Level Error
        throw
    }
}

function Invoke-VisualStudioEnvironment {
    <#
    .SYNOPSIS
        Load Visual Studio 2022 Developer PowerShell environment by calling the dedicated script.
    #>
    Write-Log "`n=== Loading Visual Studio Environment ===" -Level Info

    $setMsvcEnvScript = Join-Path $script:projectRoot "scripts\set_msvc_compiler_env.ps1"

    if (-not (Test-FileAccess -Path $setMsvcEnvScript -PathType Leaf)) {
        Write-Log "MSVC compiler environment script not found!" -Level Error
        Write-Log "Path: $setMsvcEnvScript" -Level Info
        throw "MSVC compiler environment script not found"
    }

    try {
        & $setMsvcEnvScript
        Write-Log "Visual Studio environment loaded successfully via set_msvc_compiler_env.ps1!" -Level Success
    }
    catch {
        Write-Log "Failed to load Visual Studio environment!" -Level Error
        Write-Log "Error: $_" -Level Error
        throw
    }
}

function Invoke-CMakeConfigure {
    <#
    .SYNOPSIS
        Configure the project using CMake with the specified preset.
    #>
    Write-Log "`n=== Configuring CMake ===" -Level Info

    try {
        $arguments = @(
            "--preset", $script:presetName
        )

        Write-Log "Executing: $script:cmakePath $($arguments -join ' ')" -Level Info

        & $script:cmakePath $arguments

        if ($LASTEXITCODE -ne 0) {
            Write-Log "CMake configuration failed with exit code: $LASTEXITCODE" -Level Error
            throw "CMake configuration failed"
        }

        Write-Log "CMake configuration completed successfully!" -Level Success
    }
    catch {
        Write-Log "CMake configuration encountered an error!" -Level Error
        Write-Log "Error: $_" -Level Error
        throw
    }
}

function Invoke-CMakeBuild {
    <#
    .SYNOPSIS
        Build the project using CMake.
    #>
    Write-Log "`n=== Building Project ===" -Level Info

    if (-not (Test-FileAccess -Path $script:buildDir -PathType Container)) {
        Write-Log "Build directory not found: $script:buildDir" -Level Error
        Write-Log "Please run configuration first (Action: configure or both)." -Level Info
        throw "Build directory not found"
    }

    try {
        $arguments = @(
            "--build",
            "--preset", $script:presetName
        )

        if ($script:target -ne "all") {
            $arguments += "--target", $script:target
        }

        # Ninja automatically uses all cores, but we can pass it if needed.
        # Presets might define native tool options.
        # $arguments += "-j", $script:maxThreads

        Write-Log "Executing: $script:cmakePath $($arguments -join ' ')" -Level Info

        & $script:cmakePath $arguments

        if ($LASTEXITCODE -ne 0) {
            Write-Log "Build failed with exit code: $LASTEXITCODE" -Level Error
            throw "Build failed"
        }

        $outputDir = Join-Path $script:buildDir "runtime\$script:presetName"
        $executablePath = Join-Path $outputDir "$script:target.exe"

        if ($script:target -ne "all" -and (Test-FileAccess -Path $executablePath -PathType Leaf)) {
            Write-Log "Executable created: $executablePath" -Level Success
        }

        Write-Log "Build completed successfully!" -Level Success
    }
    catch {
        Write-Log "Build encountered an error!" -Level Error
        Write-Log "Error: $_" -Level Error
        throw
    }
}

function Invoke-CMakeTest {
    <#
    .SYNOPSIS
        Run tests using CTest.
    #>
    Write-Log "`n=== Running Tests ===" -Level Info

    if (-not (Test-FileAccess -Path $script:buildDir -PathType Container)) {
        Write-Log "Build directory not found: $script:buildDir" -Level Error
        Write-Log "Please run build first (Action: build or both)." -Level Info
        throw "Build directory not found"
    }

    try {
        $ctestPath = Join-Path (Split-Path $script:cmakePath -Parent) "ctest.exe"

        if (-not (Test-FileAccess -Path $ctestPath -PathType Leaf)) {
            Write-Log "CTest executable not found at: $ctestPath" -Level Error
            throw "CTest not found"
        }

        $arguments = @(
            "--preset", $script:presetName
        )

        Write-Log "Executing: $ctestPath $($arguments -join ' ')" -Level Info

        & $ctestPath $arguments

        if ($LASTEXITCODE -ne 0) {
            Write-Log "Tests failed with exit code: $LASTEXITCODE" -Level Error
            throw "Tests failed"
        }

        Write-Log "All tests passed successfully!" -Level Success
    }
    catch {
        Write-Log "Test execution encountered an error!" -Level Error
        Write-Log "Error: $_" -Level Error
        throw
    }
}

function Invoke-CMakeInstall {
    <#
    .SYNOPSIS
        Install the project using CMake.
    #>
    Write-Log "`n=== Installing Project ===" -Level Info

    if (-not (Test-FileAccess -Path $script:buildDir -PathType Container)) {
        Write-Log "Build directory not found: $script:buildDir" -Level Error
        Write-Log "Please run build first (Action: build or both)." -Level Info
        throw "Build directory not found"
    }

    try {
        $arguments = @(
            "--install", $script:buildDir
        )

        Write-Log "Executing: $script:cmakePath $($arguments -join ' ')" -Level Info

        & $script:cmakePath $arguments

        if ($LASTEXITCODE -ne 0) {
            Write-Log "Installation failed with exit code: $LASTEXITCODE" -Level Error
            throw "Installation failed"
        }

        Write-Log "Installation completed successfully!" -Level Success
    }
    catch {
        Write-Log "Installation encountered an error!" -Level Error
        Write-Log "Error: $_" -Level Error
        throw
    }
}

function Invoke-CMakePackage {
    <#
    .SYNOPSIS
        Package the project using CPack.
    #>
    Write-Log "`n=== Packaging Project ===" -Level Info

    if (-not (Test-FileAccess -Path $script:buildDir -PathType Container)) {
        Write-Log "Build directory not found: $script:buildDir" -Level Error
        Write-Log "Please run build first (Action: build or both)." -Level Info
        throw "Build directory not found"
    }

    try {
        $cpackPath = Join-Path (Split-Path $script:cmakePath -Parent) "cpack.exe"

        if (-not (Test-FileAccess -Path $cpackPath -PathType Leaf)) {
            Write-Log "CPack executable not found at: $cpackPath" -Level Error
            throw "CPack not found"
        }

        $arguments = @(
            "-V",
            "--config", (Join-Path $script:buildDir "CPackConfig.cmake")
        )

        Write-Log "Executing: $cpackPath $($arguments -join ' ')" -Level Info

        & $cpackPath $arguments

        if ($LASTEXITCODE -ne 0) {
            Write-Log "Packaging failed with exit code: $LASTEXITCODE" -Level Error
            throw "Packaging failed"
        }

        Write-Log "Packaging completed successfully!" -Level Success
    }
    catch {
        Write-Log "Packaging encountered an error!" -Level Error
        Write-Log "Error: $_" -Level Error
        throw
    }
}

function Invoke-Main {
    <#
    .SYNOPSIS
        Main entry point for the build script.
    #>
    try {
        Write-Log "`n========================================" -Level Info
        Write-Log "FaceFusionCpp Build Script" -Level Info
        Write-Log "========================================" -Level Info

        Initialize-Environment
        Invoke-VisualStudioEnvironment

        switch ($Action) {
            "configure" {
                Invoke-CMakeConfigure
            }
            "build" {
                Invoke-CMakeBuild
            }
            "test" {
                if (-not $SkipBuild) {
                    Invoke-CMakeBuild
                }
                Invoke-CMakeTest
            }
            "install" {
                Invoke-CMakeInstall
            }
            "package" {
                Invoke-CMakePackage
            }
            "both" {
                Invoke-CMakeConfigure
                Invoke-CMakeBuild
            }
        }

        Write-Log "`n========================================" -Level Success
        Write-Log "All operations completed successfully!" -Level Success
        Write-Log "========================================" -Level Success
    }
    catch {
        Write-Log "`n========================================" -Level Error
        Write-Log "Build script failed!" -Level Error
        Write-Log "========================================" -Level Error
        Write-Log "Error: $_" -Level Error
        exit 1
    }
}

Invoke-Main
