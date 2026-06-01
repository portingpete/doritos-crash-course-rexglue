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
