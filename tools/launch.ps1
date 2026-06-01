param(
    [ValidateSet("Debug", "RelWithDebInfo", "Release")]
    [string]$Config = "Debug",
    [string]$GameRoot = "assets\game",
    [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$Root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$GameRootPath = if ([System.IO.Path]::IsPathRooted($GameRoot)) {
    [System.IO.Path]::GetFullPath($GameRoot)
}
else {
    [System.IO.Path]::GetFullPath((Join-Path $Root $GameRoot))
}
$Exe = Join-Path $Root "build\bin\$Config\doritos_port.exe"
$GeneratedSources = Join-Path $Root "port\generated\sources.cmake"
$RequiredGameFiles = @(
    "default.xex_uncrypted.xex",
    "ArcadeInfo.xml",
    "data.pak",
    "game.png",
    "game_bkgnd.jpg",
    "game_boxart.jpg",
    "game_marketplace.png"
)

function Write-Step([string]$Message) {
    Write-Host "[Doritos Launcher] $Message"
}

function Test-RequiredGameFiles {
    if (-not (Test-Path -LiteralPath $GameRootPath -PathType Container)) {
        throw "Game files folder is missing: $GameRootPath"
    }

    $missing = @()
    foreach ($file in $RequiredGameFiles) {
        $path = Join-Path $GameRootPath $file
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            $missing += $file
        }
    }

    if ($missing.Count -gt 0) {
        throw "Missing required game files in ${GameRootPath}: $($missing -join ', ')"
    }
}

Push-Location $Root
try {
    Write-Step "Checking local game files..."
    Test-RequiredGameFiles

    if (-not (Test-Path -LiteralPath $Exe -PathType Leaf)) {
        if ($SkipBuild) {
            throw "Executable not found: $Exe"
        }
        if (-not (Test-Path -LiteralPath $GeneratedSources -PathType Leaf)) {
            throw "Executable not found and generated sources are missing: $GeneratedSources. Run tools\codegen.ps1 after adding local game files, then run this launcher again."
        }
        Write-Step "Executable not found. Building $Config..."
        powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\build.ps1 -Config $Config
    }

    if (-not (Test-Path -LiteralPath $Exe -PathType Leaf)) {
        throw "Build finished, but executable is still missing: $Exe"
    }

    Write-Step "Starting Doritos Crash Course."
    Write-Step "Keyboard controls: hold Space for Xbox A; Escape maps to Start."
    powershell -NoProfile -ExecutionPolicy Bypass -File .\tools\run.ps1 `
        -Config $Config `
        -GameRoot $GameRootPath `
        -LogLevel info `
        -WindowMode windowed `
        -GraphicsBackend d3d12 `
        -AudioBackend nop `
        -NetworkingMode disabled `
        -Visible `
        -MnkMode `
        -NoConsole `
        -WindowWidth 1280 `
        -WindowHeight 720 `
        -TimeoutSeconds 0
}
finally {
    Pop-Location
}
