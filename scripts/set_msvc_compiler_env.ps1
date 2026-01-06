# Function to detect Visual Studio installation
function Get-VsInstallation {
    # Try to find VS installation using vswhere (standard tool for finding VS installations)
    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"

    if (Test-Path $vswhere) {
        $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
        if ($vsPath -and (Test-Path $vsPath)) {
            return $vsPath
        }
    }

    # Fallback: Check common installation paths
    $commonPaths = @(
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Enterprise",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Professional",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\Community",
        "${env:ProgramFiles(x86)}\Microsoft Visual Studio\2022\BuildTools"
    )

    foreach ($path in $commonPaths) {
        if (Test-Path $path) {
            return $path
        }
    }

    return $null
}

# Detect system architecture
function Get-SystemArchitecture {
    $procArch = $env:PROCESSOR_ARCHITECTURE
    $procArchW6432 = $env:PROCESSOR_ARCHITEW6432

    if ($procArchW6432) {
        $targetArch = $procArchW6432
    } else {
        $targetArch = $procArch
    }

    # Map Windows processor architecture to MSVC architecture
    switch ($targetArch) {
        { $_ -in "AMD64", "x64" } { return "x64" }
        "x86" { return "x86" }
        "ARM64" { return "arm64" }
        "ARM" { return "arm" }
        default { return "x64" }  # Default to x64 if unknown
    }
}

# Get VS installation path
$vsInstallPath = Get-VsInstallation

if (-not $vsInstallPath) {
    Write-Error "Could not find Visual Studio installation. Please install Visual Studio or Build Tools."
    exit 1
}

# Get system architecture
$arch = Get-SystemArchitecture
$hostArch = $arch  # Usually same as target arch, but can be different in cross-compilation scenarios

# Path to the DevShell module
$devShellModulePath = Join-Path $vsInstallPath "Common7\Tools\Microsoft.VisualStudio.DevShell.dll"

if (-not (Test-Path $devShellModulePath)) {
    Write-Error "Could not find DevShell module at: $devShellModulePath"
    exit 1
}

# Import the module and enter the development shell
Import-Module $devShellModulePath
Enter-VsDevShell ef10e2e8 -SkipAutomaticLocation -DevCmdArguments "-arch=$arch -host_arch=$hostArch"