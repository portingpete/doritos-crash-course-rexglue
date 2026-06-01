# MEMORY.md

## Durable Doritos Crash Course Notes

- 2026-06-01: Root workspace initially contained `game/` and
  `Rex64GlueCurrent/`; no root `resources/` directory was present.
- 2026-06-01: Input hashes were recorded before format probing:
  `default.xex` SHA-256
  `6C4FA5A12EF2AD51DB4809D5D654B487300A1ACE06C2CDF6CB14258FD273DFC1`,
  `default.xex_uncrypted.xex` SHA-256
  `EB1FB140738174B1915B01A1B776793EAB7DA518BE147F10999C9E764F3EC587`,
  `data.pak` SHA-256
  `0819B4E9F90695B70E083690E98AC6F76F0DF0CF2BBB817F196316C930A54FC3`.
- 2026-06-01: `Rex64GlueCurrent` is confirmed from its docs/notes as an
  N64Recomp-focused SDK with a known-good Conker Windows runner. Doritos/XEX
  compatibility is unconfirmed and must be proven before codegen work.
- 2026-06-01: The working Xbox 360 ReXGlue SDK for this repo is
  `K:\1943\RexGlueCurrent` version `0.7.4-dev`, not the local N64-focused
  `Rex64GlueCurrent`. CLI:
  `K:\1943\RexGlueCurrent\out\win-amd64\Debug\rexglue.exe`.
- 2026-06-01: Originals under `game/` were preserved. Runtime/copy tree is
  `assets/game/`; generated code is under `port/generated/`; codegen config is
  `port/rexglue-config/doritos_port.toml`.
- 2026-06-01: `default.xex_uncrypted.xex` metadata from XexTool: XEX2, native
  Xbox 360/Xenon PPC, unencrypted compressed retail title module, load address
  `0x82000000`, entry `0x823E1790`, image size `0x01210000`, title ID
  `58410A71`, version `v0.0.0.2`, imports `xam.xex` and `xboxkrnl.exe`.
- 2026-06-01: `data.pak` format: signature `tongas_pack_v30000`, table starts
  at `0x26`, ends near `0x146E4`, 1612 entries, LZMA-alone payloads.
- 2026-06-01: ReXGlue codegen completed successfully in 159.962 seconds:
  26,255 ready functions, 26,010 recompiled functions, 81 generated files.
  Runtime later registered 26,500 recompiled functions.
- 2026-06-01: Generated-source Debug build succeeded:
  `build/bin/Debug/doritos_port.exe`.
- 2026-06-01: Stable runtime state confirmed with D3D12, NOP audio, disabled
  networking, `--mnk_mode=true`, and held Space for Xbox A. The app advanced
  from title to country/level selection and remained alive through capture plus
  an extra 10 seconds. Screenshot:
  `logs/screenshots/doritos-heldinput-20260531-223653.png`; log:
  `logs/runtime/runtime-heldinput-20260531-223653.log`. Checked log scan found
  no fatal, exception, crash, unimplemented, assert, or failed draw diagnostics.
- 2026-06-01: Caveat: actual obstacle-course gameplay is not yet confirmed.
  Quick key taps can be missed; held input is the known-good verification path.
- 2026-06-01: Added easy launcher: root `Launch Doritos Crash Course.bat`
  calls `tools/launch.ps1`. It verifies `assets/game/`, builds Debug when
  needed, then runs the port visibly with D3D12, NOP audio, disabled
  networking, MnK enabled, and a 1280x720 window.
- 2026-06-01: Xbox Live profile requirement removed with project-local XAM
  profile shims. Generated gate research found `sub_823E0E28` in
  `port/generated/doritos_port_recomp.19.cpp` treats sign-in state `1` as
  local-only/error and checks privileges; `sub_828891F0` in
  `port/generated/doritos_port_recomp.61.cpp` reads profile settings and checks
  privileges. Added forced generated-source include
  `src/runtime/xam_profile_import_redirect.h` and runtime implementation
  `src/runtime/xam_profile_overrides.cpp`; user 0 now reports Live sign-in
  state `2`, a synthetic `User` sign-in record, allowed privileges, and no
  sign-in modal.
- 2026-06-01: Profile override verification passed:
  `tools/verify_profile_override.ps1`, Debug build, and runtime smoke log
  `logs/runtime/runtime-profile-override-20260601-095743.log`. Smoke confirmed
  the override installed, `XamUserGetSigninState` and `XamUserCheckPrivilege`
  were used, the process stayed alive, and the checked log scan had no
  fatal/assert/import/draw diagnostics. Computer Use window capture was blocked
  by Windows with `0x80070005`, so this profile smoke is log/process confirmed
  rather than screenshot confirmed.
- 2026-06-01: Start-game crash investigation used user dump
  `logs/runtime/dumps/doritos-port-crash.dmp` timestamped
  `2026-06-01 10:19:49`. Confirmed exception path rethrows guest PPC access
  violation through `rex::ppc::detail::seh_rethrow`, with generated frames in
  `sub_823E8448` and host wrapper `rex::system::XThread::Execute`. Matching
  log `logs/runtime/runtime-20260601-101910.log` shows
  `XamUserGetSigninInfo` then `XamShowSigninUI` before the crash.
- 2026-06-01: Corrected start-game profile flag finding. Doritos generated
  start-game code reads `XamUserGetSigninInfo` output word at offset `+8` and
  rejects bit `0x2` as a guest-profile condition. A user screenshot confirmed
  `0x2` shows "Guest gamer profiles are not supported". The project shim now
  writes `kSyntheticSigninInfoFlags = 0x00000001u` to offset `+8`, keeping the
  guest bit clear while preserving a Live-enabled synthetic profile marker.
- 2026-06-01: With sign-in info flags `0x1`, scripted start no longer shows the
  guest-profile dialog. Game-only capture
  `logs/screenshots/doritos-start-course-debug-20260601-115219.bmp` shows the
  "Saving..." overlay before the process crashes. Latest dump reports
  `0xC0000005` reading guest address `0x20`, rethrown through
  `rex::ppc::detail::seh_rethrow`; mapped generated frames include
  `sub_822AD9C8+0x520` and `sub_823E8448+0xffb`.
- 2026-06-01: Start-course crash root cause refined. LLDB showed the
  course-load object is stack-local in `sub_8233F360` (`r1+96`). The destructor
  path `sub_824D5258 -> sub_824D2888 -> sub_824E14C0` clears the object's
  service pointer at `this+4`; a later XThread worker then entered
  `sub_824E9940`/`sub_824EA8F8` with that pointer already null. A previous
  guard that called the `sub_824E9940` finish callback and returned `4`
  advanced into a second null dereference in `sub_824EA8F8`.
- 2026-06-01: Working course-load guard: for guest function `0x824E9940`, if
  `this+4` is null, log the stale worker once and return `0` without calling
  the finish callback. Non-null calls still pass through the original generated
  function.
- 2026-06-01: Scripted start-course verification with the stale-worker guard
  passed the narrow stability gate. The run used D3D12, NOP audio, disabled
  networking, `--mnk_mode=true`, and `--doritos_scripted_input=start_course`;
  it reached live obstacle-course gameplay, captured a game-only framebuffer at
  `logs/screenshots/doritos-start-course-pendingguard-20260601-130158.bmp`,
  and remained alive until the 100 second harness timeout. Public docs PNG:
  `docs/screenshots/04-obstacle-course-gameplay.png`; runtime log:
  `logs/runtime/runtime-start-course-pendingguard-20260601-130158.log`.
