param(
    [string]$Root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
)

$ErrorActionPreference = "Stop"

$cmakePath = Join-Path $Root "CMakeLists.txt"
$redirectPath = Join-Path $Root "src\runtime\xam_profile_import_redirect.h"
$overrideHeaderPath = Join-Path $Root "src\runtime\xam_profile_overrides.h"
$overridePath = Join-Path $Root "src\runtime\xam_profile_overrides.cpp"
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
Require-File $overrideHeaderPath "Profile override header" | Out-Null
Require-File $overridePath "Profile override implementation" | Out-Null
Require-File $mainPath "Main app" | Out-Null

Require-Text $cmakePath "xam_profile_overrides.cpp" "CMake"
Require-Text $cmakePath "xam_profile_import_redirect.h" "CMake"
Require-Text $cmakePath "-include" "CMake"
Require-Text $redirectPath "#define __imp__XamUserGetSigninState doritos_XamUserGetSigninState" "Import redirect header"
Require-Text $redirectPath "#define __imp__XamUserGetSigninInfo doritos_XamUserGetSigninInfo" "Import redirect header"
Require-Text $redirectPath "#define __imp__XamUserCheckPrivilege doritos_XamUserCheckPrivilege" "Import redirect header"
Require-Text $redirectPath "#define __imp__XamShowSigninUI doritos_XamShowSigninUI" "Import redirect header"
Require-Text $overridePath "kLiveSigninState = 2" "Profile override implementation"
Require-Text $overridePath "doritos_XamUserGetSigninState" "Profile override implementation"
Require-Text $overridePath "doritos_XamUserGetSigninInfo" "Profile override implementation"
Require-Text $overridePath "doritos_XamUserCheckPrivilege" "Profile override implementation"
Require-Text $overridePath "doritos_XamShowSigninUI" "Profile override implementation"
Require-Text $overridePath "PPC_STORE_U32(ctx.r5.u32, 1)" "Profile override implementation"
Require-Text $overridePath "InstallXamProfileOverrides" "Profile override implementation"
Require-Text $mainPath "InstallXamProfileOverrides()" "Main app"

if ($failures.Count -gt 0) {
    $failures | ForEach-Object { Write-Error $_ }
    exit 1
}

Write-Host "Profile override verification passed."
