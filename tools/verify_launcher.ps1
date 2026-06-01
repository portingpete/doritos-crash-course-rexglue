param(
    [string]$Root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
)

$ErrorActionPreference = "Stop"

$batchPath = Join-Path $Root "Launch Doritos Crash Course.bat"
$launcherPath = Join-Path $Root "tools\launch.ps1"
$readmePath = Join-Path $Root "README.md"

$failures = New-Object System.Collections.Generic.List[string]

function Require-File([string]$Path, [string]$Label) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        $script:failures.Add("$Label missing: $Path")
        return $false
    }
    return $true
}

function Require-Text([string]$Path, [string]$Needle, [string]$Label) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        return
    }
    $content = Get-Content -LiteralPath $Path -Raw
    if ($content -notlike "*$Needle*") {
        $script:failures.Add("$Label missing text: $Needle")
    }
}

Require-File $batchPath "Batch launcher" | Out-Null
Require-File $launcherPath "PowerShell launcher" | Out-Null
Require-File $readmePath "README" | Out-Null

Require-Text $batchPath "tools\launch.ps1" "Batch launcher"
Require-Text $launcherPath "tools\build.ps1" "PowerShell launcher"
Require-Text $launcherPath "tools\run.ps1" "PowerShell launcher"
Require-Text $launcherPath "default.xex_uncrypted.xex" "PowerShell launcher"
Require-Text $launcherPath "data.pak" "PowerShell launcher"
Require-Text $launcherPath "port\generated\sources.cmake" "PowerShell launcher"
Require-Text $launcherPath "Run tools\codegen.ps1" "PowerShell launcher"
Require-Text $launcherPath "-GraphicsBackend d3d12" "PowerShell launcher"
Require-Text $launcherPath "-AudioBackend nop" "PowerShell launcher"
Require-Text $launcherPath "-NetworkingMode disabled" "PowerShell launcher"
Require-Text $launcherPath "-MnkMode" "PowerShell launcher"
Require-Text $launcherPath "-Visible" "PowerShell launcher"
Require-Text $launcherPath "-NoConsole" "PowerShell launcher"
Require-Text $launcherPath "-WindowWidth 1280" "PowerShell launcher"
Require-Text $launcherPath "-WindowHeight 720" "PowerShell launcher"
Require-Text $launcherPath "-TimeoutSeconds 0" "PowerShell launcher"
Require-Text $readmePath "Launch Doritos Crash Course.bat" "README"

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Host "Launcher verification passed."
