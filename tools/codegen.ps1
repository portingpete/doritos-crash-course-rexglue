param(
    [string]$ConfigPath = "port\rexglue-config\doritos_port.toml",
    [ValidateSet("Debug", "RelWithDebInfo", "Release")]
    [string]$BuildConfig = "Debug",
    [ValidateSet("error", "warning", "info", "debug", "trace")]
    [string]$LogLevel = "debug",
    [int]$TimeoutSeconds = 3600,
    [switch]$Force
)

$ErrorActionPreference = "Stop"
$Root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$RexGlueExeCandidates = @(
    (Join-Path $Root "RexGlueCurrent\out\win-amd64\$BuildConfig\rexglue.exe"),
    (Join-Path $Root "RexGlueCurrent\out\win-amd64\rexglue.exe"),
    "K:\1943\RexGlueCurrent\out\win-amd64\$BuildConfig\rexglue.exe",
    "K:\1943\RexGlueCurrent\out\win-amd64\rexglue.exe"
)
$RexGlueExe = $RexGlueExeCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
$FullConfigPath = Join-Path $Root $ConfigPath
$LogDir = Join-Path $Root "logs\codegen"
$Stamp = Get-Date -Format "yyyyMMdd-HHmmss"
$LogFile = Join-Path $LogDir "codegen-$Stamp.log"
$StdoutFile = Join-Path $LogDir "codegen-$Stamp.stdout.txt"
$StderrFile = Join-Path $LogDir "codegen-$Stamp.stderr.txt"

New-Item -ItemType Directory -Force -Path $LogDir | Out-Null

if (-not $RexGlueExe) {
    throw "Local ReXGlue CLI not found. Checked: $($RexGlueExeCandidates -join ', '). Build RexGlueCurrent first; do not use a global rexglue command."
}
if (-not (Test-Path -LiteralPath $FullConfigPath)) {
    throw "ReXGlue config not found: $FullConfigPath"
}
if (-not (Test-Path -LiteralPath (Join-Path $Root "assets\game\default.xex_uncrypted.xex"))) {
    throw "Missing assets\game\default.xex_uncrypted.xex. Run tools\verify_game_files.py for details."
}

Push-Location $Root
try {
    $args = @("codegen", $FullConfigPath, "--log_level=$LogLevel", "--log_file=$LogFile")
    if ($Force) {
        $args += "--force"
    }

    $process = Start-Process -FilePath $RexGlueExe `
        -ArgumentList $args `
        -RedirectStandardOutput $StdoutFile `
        -RedirectStandardError $StderrFile `
        -NoNewWindow `
        -PassThru

    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        Stop-Process -Id $process.Id -Force
        throw "ReXGlue codegen exceeded ${TimeoutSeconds}s and was stopped. See $StdoutFile, $StderrFile, and $LogFile"
    }

    $process.Refresh()
    $exitCode = $process.ExitCode
    if ($null -eq $exitCode) {
        $completed = (Select-String -LiteralPath $StdoutFile, $LogFile -Pattern "Operation completed successfully" -SimpleMatch -Quiet)
        if ($completed) {
            $exitCode = 0
        }
    }

    if ($exitCode -ne 0) {
        throw "ReXGlue codegen failed with exit $exitCode. See $StdoutFile, $StderrFile, and $LogFile"
    }
    Write-Host "ReXGlue CLI: $RexGlueExe"
    Write-Host "Codegen log: $LogFile"
    Write-Host "Codegen stdout: $StdoutFile"
    Write-Host "Codegen stderr: $StderrFile"
}
finally {
    Pop-Location
}

