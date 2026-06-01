param(
    [ValidateSet("Debug", "RelWithDebInfo", "Release")]
    [string]$Config = "Debug",
    [string]$GameRoot = "assets\\game",
    [ValidateSet("error", "warning", "info", "debug", "trace")]
    [string]$LogLevel = "info",
    [ValidateSet("windowed", "fullscreen")]
    [string]$WindowMode = "windowed",
    [ValidateSet("d3d12", "vulkan", "none")]
    [string]$GraphicsBackend = "d3d12",
    [ValidateSet("sdl", "nop")]
    [string]$AudioBackend = "sdl",
    [ValidateSet("disabled", "stubbed", "lan", "service")]
    [string]$NetworkingMode = "disabled",
    [switch]$Visible,
    [switch]$MnkMode,
    [switch]$NoConsole,
    [int]$WindowWidth = 0,
    [int]$WindowHeight = 0,
    [int]$TimeoutSeconds = 300
)

$ErrorActionPreference = "Stop"
$Root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$Exe = Join-Path $Root "build\bin\$Config\doritos_port.exe"
$LogDir = Join-Path $Root "logs\runtime"
$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$LogFile = Join-Path $LogDir "runtime-$Stamp.log"

New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

if (-not (Test-Path -LiteralPath $Exe)) {
    throw "Port executable not found: $Exe. Run tools\build.ps1 -Config $Config first."
}

$env:DORITOS_GRAPHICS_BACKEND = $GraphicsBackend
$env:DORITOS_AUDIO_BACKEND = $AudioBackend
$env:DORITOS_NETWORKING_MODE = $NetworkingMode

$fullscreen = if ($WindowMode -eq "fullscreen") { "true" } else { "false" }

Push-Location $Root
try {
    $args = @(
        "--log_level=$LogLevel",
        "--log_file=$LogFile",
        "--fullscreen=$fullscreen",
        "--enable_console=$(if ($NoConsole) { 'false' } else { 'true' })",
        "--mnk_mode=$(if ($MnkMode) { 'true' } else { 'false' })",
        $GameRoot
    )
    if ($WindowWidth -gt 0) {
        $args = @("--window_width=$WindowWidth") + $args
    }
    if ($WindowHeight -gt 0) {
        $args = @("--window_height=$WindowHeight") + $args
    }
    $windowStyle = if ($Visible) { "Normal" } else { "Hidden" }
    $process = Start-Process -FilePath $Exe -ArgumentList $args -PassThru -WindowStyle $windowStyle
    if ($TimeoutSeconds -gt 0) {
        $exited = $process.WaitForExit($TimeoutSeconds * 1000)
        if (-not $exited) {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
            throw "Port did not exit within $TimeoutSeconds seconds. Process was stopped. See $LogFile"
        }
    }
    else {
        $process.WaitForExit()
    }
    if ($process.ExitCode -ne 0) {
        throw "Port exited with $($process.ExitCode). See $LogFile"
    }
    Write-Host "Runtime log: $LogFile"
}
finally {
    Pop-Location
}

