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
- Input: MnK bridge enabled; held Space maps to Xbox A

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

## Confirmed Stable State

The current confirmed stable in-game state is the title screen advancing into country/level selection. The process stayed alive through capture and additional runtime, and checked diagnostics did not show fatal errors, exceptions, unimplemented calls, assertions, crashes, or failed draw dispatches.

## Caveats

- Actual obstacle-course gameplay has not yet been separately confirmed.
- Full audio has not been signed off; stable verification used the NOP audio backend.
- Held keyboard input was needed for reliable Xbox A activation. Quick taps can be missed by the current MnK bridge.

