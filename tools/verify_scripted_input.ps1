param(
    [string]$Root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
)

$ErrorActionPreference = "Stop"

$cmakePath = Join-Path $Root "CMakeLists.txt"
$redirectPath = Join-Path $Root "src\runtime\xam_profile_import_redirect.h"
$inputHeaderPath = Join-Path $Root "src\runtime\xam_input_overrides.h"
$inputPath = Join-Path $Root "src\runtime\xam_input_overrides.cpp"
$captureHeaderPath = Join-Path $Root "src\diagnostics\guest_capture.h"
$capturePath = Join-Path $Root "src\diagnostics\guest_capture.cpp"
$courseGuardHeaderPath = Join-Path $Root "src\runtime\course_load_guard.h"
$courseGuardPath = Join-Path $Root "src\runtime\course_load_guard.cpp"
$mainPath = Join-Path $Root "src\main.cpp"

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

Require-File $cmakePath "CMake" | Out-Null
Require-File $redirectPath "Import redirect header" | Out-Null
Require-File $inputHeaderPath "Input override header" | Out-Null
Require-File $inputPath "Input override implementation" | Out-Null
Require-File $captureHeaderPath "Guest capture header" | Out-Null
Require-File $capturePath "Guest capture implementation" | Out-Null
Require-File $courseGuardHeaderPath "Course load guard header" | Out-Null
Require-File $courseGuardPath "Course load guard implementation" | Out-Null
Require-File $mainPath "Main app" | Out-Null

Require-Text $cmakePath "xam_input_overrides.cpp" "CMake"
Require-Text $cmakePath "guest_capture.cpp" "CMake"
Require-Text $cmakePath "course_load_guard.cpp" "CMake"
Require-Text $redirectPath "#define __imp__XamInputGetState doritos_XamInputGetState" "Import redirect header"
Require-Text $inputPath "REXCVAR_DEFINE_STRING(doritos_scripted_input" "Input override implementation"
Require-Text $inputPath "start_course" "Input override implementation"
Require-Text $inputPath "ScriptedButtonsForElapsedMs" "Input override implementation"
Require-Text $inputPath "X_INPUT_GAMEPAD_A" "Input override implementation"
Require-Text $inputPath "doritos_XamInputGetState" "Input override implementation"
Require-Text $inputPath "InstallXamInputOverrides" "Input override implementation"
Require-Text $capturePath "REXCVAR_DEFINE_BOOL(doritos_capture_guest_output" "Guest capture implementation"
Require-Text $capturePath "REXCVAR_DEFINE_STRING(doritos_capture_guest_output_path" "Guest capture implementation"
Require-Text $capturePath "CaptureGuestOutput" "Guest capture implementation"
Require-Text $capturePath "SaveBmp" "Guest capture implementation"
Require-Text $capturePath "StartGuestOutputCapture" "Guest capture implementation"
Require-Text $courseGuardPath "kTileWorkerStepAddress = 0x824E9940u" "Course load guard implementation"
Require-Text $courseGuardPath "stale/null service" "Course load guard implementation"
Require-Text $courseGuardPath "ctx.r3.s64 = 0" "Course load guard implementation"
Require-Text $courseGuardPath "SetFunction(kTileWorkerStepAddress" "Course load guard implementation"
Require-Text $mainPath "InstallXamInputOverrides()" "Main app"
Require-Text $mainPath "StartGuestOutputCapture" "Main app"
Require-Text $mainPath "InstallCourseLoadGuard" "Main app"
Require-Text $redirectPath "#define __imp__XamInputGetKeystroke doritos_XamInputGetKeystroke" "Import redirect header"
Require-Text $redirectPath "#define __imp__XamInputGetKeystrokeEx doritos_XamInputGetKeystrokeEx" "Import redirect header"
Require-Text $inputPath "doritos_XamInputGetKeystroke" "Input override implementation"
Require-Text $inputPath "doritos_XamInputGetKeystrokeEx" "Input override implementation"
Require-Text $inputPath "kXInputPadA" "Input override implementation"

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Host "Scripted input verification passed."
