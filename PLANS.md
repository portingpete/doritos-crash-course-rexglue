# Doritos Crash Course Windows Recompile Plan

Goal: use the RexGlue/Rex64Glue project in this workspace to produce a
Windows-runnable Doritos Crash Course build and reach a stable in-game state.

## Constraints

- Preserve originals under `game/`; use copied/extracted working files only.
- Track hashes, formats, offsets, symbols, signatures, changed files, and
  verification results when relevant.
- Separate confirmed facts from inference.
- Reuse known-good Rex64Glue workflows when they fit; avoid forcing N64-only
  assumptions onto Xbox 360/XEX inputs.
- Never submit GitHub PRs or issues.

## Current Confirmed Inputs

- `game/default.xex`: 4,501,504 bytes, SHA-256
  `6C4FA5A12EF2AD51DB4809D5D654B487300A1ACE06C2CDF6CB14258FD273DFC1`.
- `game/default.xex_uncrypted.xex`: 4,501,504 bytes, SHA-256
  `EB1FB140738174B1915B01A1B776793EAB7DA518BE147F10999C9E764F3EC587`.
- `game/data.pak`: 55,672,832 bytes, SHA-256
  `0819B4E9F90695B70E083690E98AC6F76F0DF0CF2BBB817F196316C930A54FC3`.
- `ArcadeInfo.xml`: title Doritos Crash Course, Title ID `58410A71`,
  project version `1.0.273.0`, executable path `default.xex`.
- `game/default.xex_uncrypted.xex`: XEX2, native Xbox 360/Xenon PPC image,
  load address `0x82000000`, entry `0x823E1790`, image size `0x01210000`,
  imports `xam.xex` and `xboxkrnl.exe`.
- `game/data.pak`: signature `tongas_pack_v30000`; table begins at `0x26`,
  ends near `0x146E4`, contains 1612 path entries with LZMA-alone payloads.
- No root `resources/` directory is present.

## Current Confirmed Outputs

- Originals under `game/` were not edited. The runnable asset copy is
  `assets/game/`.
- ReXGlue used: `K:\1943\RexGlueCurrent`, version `0.7.4-dev`; CLI
  `K:\1943\RexGlueCurrent\out\win-amd64\Debug\rexglue.exe`.
- Codegen config: `port/rexglue-config/doritos_port.toml`; generated output:
  `port/generated/`.
- Codegen completed successfully in 159.962 seconds, producing 81 generated
  files. Analysis found 26,255 ready functions and recompiled 26,010 functions.
  Runtime registered 26,500 recompiled functions.
- Build succeeded for `build/bin/Debug/doritos_port.exe` with generated sources.
- Stable runtime verification reached title, then country/level selection using
  `--mnk_mode=true` and a held Space key for Xbox A.
- Xbox Live profile requirement patch verified by static check, Debug build,
  and focused runtime smoke. Log:
  `logs/runtime/runtime-profile-override-20260601-095743.log`; process stayed
  alive, installed the Doritos profile override, reported user 0 as Live
  signed in, and allowed the privilege check with no fatal/assert/import/draw
  diagnostics in the checked scan.
- Verification screenshots:
  `logs/screenshots/doritos-input-20260531-223134.png`,
  `logs/screenshots/doritos-heldinput-20260531-223653.png`.
- Best runtime proof log:
  `logs/runtime/runtime-heldinput-20260531-223653.log`; process stayed alive
  through capture and an extra 10 seconds, and the log had no fatal, exception,
  crash, unimplemented, assert, or failed draw diagnostics in the checked scan.

## Current Caveats

- Confirmed stable state is the title and country/level selection flow, not a
  loaded obstacle course run.
- Quick keyboard taps may be missed; held input through MnK was needed for the
  title `Press A` prompt. Default MnK mapping: Space = A, Escape = Start.
- Audio was verified with the NOP backend for stability; full audio has not
  been signed off.

## Milestones

- [x] Identify the executable and asset formats without modifying originals.
      Record headers, signatures, likely architecture/runtime, and extraction
      options.
- [x] Decide whether Rex64Glue can be extended directly or whether a
      Doritos-specific Windows lane must be added beside the N64Recomp path.
- [x] Create a working copy/extraction tree for generated or modified files.
- [x] Build the narrowest Windows host that can load game assets and reach a
      deterministic runtime checkpoint.
- [x] Iterate missing platform/runtime shims until the build reaches a stable
      in-game state.
- [x] Run the narrowest useful verification and record durable findings in
      `AGENTS.md` and `MEMORY.md`.

## Verification Gates

- [x] Format probe: report executable magic/architecture/runtime and `data.pak`
  container signature with offsets.
- [x] Build gate: compile the Windows target from copied/generated files.
- [x] Runtime gate: launch the Windows target long enough to reach stable in-game
  state without fatal exception, access violation, assertion, or missing
  critical symbol diagnostics.

## Xbox Live Profile Requirement Patch

Goal: remove the title's Xbox Live profile gate without editing `game/`,
`assets/game/`, `port/generated/`, or the external ReXGlue SDK in place.

- [x] Confirm the gate by tracing local XAM profile imports and generated call
  sites.
- [x] Add a Doritos-local import redirect for generated code so selected XAM
  calls see a synthetic signed-in Live-capable user.
- [x] Verify the override statically, rebuild, and run a focused runtime smoke
  test to confirm the title uses the synthetic Live profile path without a
  sign-in block.
- [x] Record durable findings in `AGENTS.md` and `MEMORY.md`.

## Start Game Crash Patch

Goal: fix the crash reported when starting a game after removing the Xbox Live
profile gate, while keeping the change project-local and preserving clean
binaries/assets.

- [x] Collect the latest runtime log/dump evidence and stack context.
- [x] Trace generated start-game sign-in info call sites.
- [x] Add a regression check for the synthetic sign-in info flags word.
- [x] Patch the synthetic profile record to report the Live-capable info bit.
- [x] Rebuild/run the narrowest useful verification and record durable results.
