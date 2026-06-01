# Current Performance and Stability

Date: 2026-06-01

## Test Environment

- Host: Windows x64
- GPU observed by runtime: NVIDIA GeForce RTX 3080 Ti
- ReXGlue SDK: `K:\1943\RexGlueCurrent`, version `0.7.4-dev`
- Build: Debug
- Graphics backend: D3D12
- Audio backend: NOP
- Networking: disabled
- Window: 1280x720
- Input: MnK bridge enabled; scripted course-start verification uses the
  project-local XAM input shim

## Codegen and Build

- Codegen completed successfully in 159.962 seconds.
- Analysis produced 26,255 ready functions.
- ReXGlue recompiled 26,010 guest functions.
- Generated output contains 81 files, about 156.9 MB of generated C++.
- Runtime registered 26,500 recompiled functions.
- Debug executable linked successfully as `build/bin/Debug/doritos_port.exe`.

## Runtime Measurements

Measurements below are from runtime logs. Draw dispatch count is not FPS; it is the GPU command processor's reported draw dispatch total.

| Run | Log | Duration | State Reached | Last Draw Dispatch | Failed Draws |
| --- | --- | ---: | --- | ---: | ---: |
| Held input selection | `logs/runtime/runtime-heldinput-20260531-223653.log` | 105.471s | Title to country/level selection | 641,280 | 0 |
| Longer stability probe | `logs/runtime/runtime-gameplay-20260531-223932.log` | 143.397s | Country/level selection remained live | 874,176 | 0 |
| Title prompt probe | `logs/runtime/runtime-input-20260531-223134.log` | 53.510s | Title prompt with live rendering | 286,240 | 0 |

## Course Gameplay Verification

The latest start-course verification used D3D12 rendering, NOP audio, disabled
networking, a 1280x720 window, and the scripted input shim. The process reached
live obstacle-course gameplay, produced a game-only framebuffer capture at
`logs/screenshots/doritos-start-course-pendingguard-20260601-130158.bmp`, and
was still alive after the 100 second harness timeout. The harness then stopped
the process intentionally.

- Public screenshot:
  `docs/screenshots/04-obstacle-course-gameplay.png`
- Runtime log:
  `logs/runtime/runtime-start-course-pendingguard-20260601-130158.log`
- Observable game timer at capture: `00:28.626`
- Crash diagnostics: no first-chance exception, unhandled exception, fatal,
  assert, unimplemented, crash, or failed draw lines were found in the checked
  log scan after the guard fired.
- FPS telemetry: not emitted by this verification run.

## Confirmed Stable State

The current confirmed stable in-game state is obstacle-course gameplay. The
process stayed alive through capture and additional runtime, and checked
diagnostics did not show fatal errors, exceptions, unimplemented calls,
assertions, crashes, or failed draw dispatches.

## Caveats

- Full audio has not been signed off; stable verification used the NOP audio backend.
- Held keyboard input was needed for reliable Xbox A activation. Quick taps can be missed by the current MnK bridge.
