# AGENTS.md

## Repo Rules

- Preserve originals under `game/`; work on copies and generated/extracted
  trees only.
- Consult `AGENTS.md`, `MEMORY.md`, and `PLANS.md` before risky changes.
- Track build/version, offsets, symbols, signatures, hashes, changed files, and
  verification results when relevant.
- Use subagents for substantial independent analysis.
- Before success claims, run the narrowest useful verification.
- Separate confirmed findings from inference.
- Keep durable notes concise and actionable.
- Never submit GitHub PRs or issues.

## Durable Findings

- Confirmed: no root `resources/` directory is present in
  `K:\DoritosCrashCourse`.
- Confirmed input hashes are recorded in `PLANS.md`.
- Confirmed: existing `Rex64GlueCurrent` documentation and notes describe an
  N64Recomp/N64ModernRuntime workflow and a known-good Conker Windows path.
- Confirmed: `Rex64GlueCurrent` is not the right path for Doritos/XEX. The
  working Xbox 360 ReXGlue SDK is `K:\1943\RexGlueCurrent`, version
  `0.7.4-dev`; use its CLI at
  `K:\1943\RexGlueCurrent\out\win-amd64\Debug\rexglue.exe`.
- Confirmed: originals under `game/` remained unchanged. Working/runnable game
  files were copied to `assets/game/`.
- Confirmed: `default.xex_uncrypted.xex` is a native Xbox 360 XEX2 image, load
  address `0x82000000`, entry `0x823E1790`, image size `0x01210000`, imports
  `xam.xex` and `xboxkrnl.exe`.
- Confirmed: `data.pak` signature is `tongas_pack_v30000`; entry table starts
  at `0x26`, ends near `0x146E4`, has 1612 path entries, and payloads are
  LZMA-alone.
- Confirmed: ReXGlue codegen using `port/rexglue-config/doritos_port.toml`
  completed in 159.962 seconds. It analyzed 26,255 ready functions, recompiled
  26,010 functions, and emitted 81 files under `port/generated/`.
- Confirmed: generated-source Windows build succeeded and linked
  `build/bin/Debug/doritos_port.exe`.
- Confirmed: runtime with D3D12, NOP audio, disabled networking, and
  `--mnk_mode=true` reached the Doritos Crash Course title screen and advanced
  to the country/level selection flow. Verification screenshot:
  `logs/screenshots/doritos-heldinput-20260531-223653.png`; verification log:
  `logs/runtime/runtime-heldinput-20260531-223653.log`.
- Confirmed: in the stable selection run the process remained alive through
  capture and an extra 10 seconds. Checked diagnostics showed no fatal,
  exception, crash, unimplemented, assert, or failed draw entries.
- Confirmed: the Xbox Live profile gate is driven by XAM profile imports rather
  than clean binary patching. Generated call sites include
  `sub_823E0E28` in `port/generated/doritos_port_recomp.19.cpp`, which treats
  local-only sign-in state `1` as an error and checks privileges, and
  `sub_828891F0` in `port/generated/doritos_port_recomp.61.cpp`, which reads
  profile settings and checks privileges.
- Confirmed: the project-local profile override redirects generated calls for
  `XamUserGetSigninState`, `XamUserGetSigninInfo`, `XamUserCheckPrivilege`, and
  `XamShowSigninUI` to Doritos shims that report user 0 as Live signed in
  (`2`), fill a synthetic `User` sign-in record, allow privilege checks, and
  suppress the sign-in modal. Verification log:
  `logs/runtime/runtime-profile-override-20260601-095743.log`.

## Repo-Specific Do/Do-Not

- Do use `assets/game/default.xex_uncrypted.xex` for ReXGlue codegen/runtime.
- Do rerun CMake configure after regenerating `port/generated/sources.cmake`.
- Do enable MnK for keyboard-controlled verification. Default useful mapping:
  Space = Xbox A, Escape = Start.
- Do treat quick key taps as unreliable; held Space through the window message
  path was the confirmed way to pass the title prompt.
- Do use `Launch Doritos Crash Course.bat` for double-click local bring-up. It
  delegates to `tools/launch.ps1`, validates `assets/game/`, builds Debug if
  needed, and launches with D3D12, NOP audio, disabled networking, MnK, and a
  1280x720 window.
- Do not claim a fully playable course yet. Confirmed stable state is title and
  country/level selection, not a loaded obstacle course run.
- Do not edit the clean `game/` files in place.
- Do keep the Xbox profile bypass project-local: use
  `src/runtime/xam_profile_import_redirect.h` and
  `src/runtime/xam_profile_overrides.cpp`; do not patch `port/generated/`, the
  external ReXGlue SDK, or clean game assets for this requirement.
