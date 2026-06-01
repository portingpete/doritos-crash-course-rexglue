param(
    [ValidateSet("Debug", "RelWithDebInfo", "Release")]
    [string]$Config = "Debug",
    [switch]$SkipConfigure
)

$ErrorActionPreference = "Stop"
$Root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$RunningOnWindows = $env:OS -eq "Windows_NT"

if ($RunningOnWindows -and -not $env:VSCMD_VER) {
    $vsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    if (Test-Path -LiteralPath $vsWhere) {
        $vsInstall = & $vsWhere -latest -property installationPath
        if ($vsInstall) {
            $launchScript = Join-Path $vsInstall "Common7\Tools\Launch-VsDevShell.ps1"
            if (Test-Path -LiteralPath $launchScript) {
                & $launchScript -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
            }
        }
    }
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
    throw "cmake was not found on PATH."
}
if (-not (Get-Command ninja -ErrorAction SilentlyContinue)) {
    throw "ninja was not found on PATH."
}
if (-not (Get-Command clang++ -ErrorAction SilentlyContinue)) {
    throw "clang++ was not found on PATH. Local ReXGlue requires Clang 18 or newer."
}

Push-Location $Root
try {
    if (-not $SkipConfigure) {
        cmake --preset win-amd64
        if ($LASTEXITCODE -ne 0) {
            throw "cmake configure failed with exit $LASTEXITCODE"
        }
    }

    $presetSuffix = $Config.ToLowerInvariant()
    cmake --build --preset "win-amd64-$presetSuffix"
    if ($LASTEXITCODE -ne 0) {
        throw "cmake build failed with exit $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}

