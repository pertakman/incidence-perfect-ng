# Hardware Validation Checklist

Use this checklist against the current firmware build to establish a known-good baseline.

## Preconditions

- Firmware uploaded from PlatformIO (`env: esp32s3`).
- Board powered over USB and responsive.
- Serial Monitor open at `115200`.
- Device resting on a stable, mostly level surface.

## Known Current UI Limitations

- None currently tracked as blocking for beta.

## Test Cases

| ID | Action | Expected Result | Pass/Fail | Notes |
|---|---|---|---|---|
| T01 | Power cycle board | UI boots, shows roll/pitch readout, touch responsive |  |  |
| T02 | Observe serial startup | Mode banner prints with orientation and sign conventions |  |  |
| T03 | Tilt right edge down slowly | `ROLL` moves positive, smooth update, color warning after ~30 deg |  |  |
| T04 | Tilt left edge down slowly | `ROLL` moves negative |  |  |
| T05 | Tilt top toward user slowly | `PITCH` moves positive |  |  |
| T06 | Tap `ZERO` once on stable surface | Enters guided ZERO (`ZERO`->`CANCEL`, `MODE`->`CONFIRM`) with hint/progress support |  |  |
| T07 | In guided ZERO press `CONFIRM` and hold still | ZERO completes; values recenter near `+0.00 deg` |  |  |
| T08 | In guided ZERO press `CANCEL` | Returns to normal mode without applying new zero |  |  |
| T09 | Long-press `ZERO` in normal mode | Enters guided RECAL (`ZERO`->`CANCEL`, `MODE`->`CONFIRM`) |  |  |
| T10 | In guided RECAL press `CONFIRM` and hold still | RECAL completes with progress bar; readings settle |  |  |
| T11 | Tap `AXIS` repeatedly | Layout cycles: both -> roll-only -> pitch-only -> both |  |  |
| T12 | Tap `ALIGN`, then `CANCEL` | Enters alignment modal, then returns to normal UI |  |  |
| T13 | Tap `ALIGN`, capture 1-2 steps, then `CANCEL` | Step text updates clearly and flow can be aborted safely |  |  |
| T14 | Tap `MODE` once | Orientation toggles immediately (`SCREEN UP` <-> `SCREEN VERTICAL`) |  |  |
| T15 | In serial monitor send `z` | Starts guided ZERO workflow |  |  |
| T16 | In serial monitor send `c` while ZERO pending | Confirms ZERO workflow and starts hold/sampling |  |  |
| T17 | In serial monitor send `k` | Starts guided RECAL workflow |  |  |
| T18 | In serial monitor send `c` while RECAL pending | Confirms RECAL workflow and starts hold/sampling |  |  |
| T19 | In serial monitor send `u` | Orientation switches immediately to `SCREEN UP` |  |  |
| T20 | In serial monitor send `v` | Orientation switches immediately to `SCREEN VERTICAL` |  |  |
| T21 | In serial monitor send `m` | Orientation toggles immediately |  |  |
| T22 | In serial monitor send `x` during ZERO/RECAL/ALIGN | Active workflow is canceled |  |  |
| T23 | Tap `ROTATE` | Full UI rotates 180 deg and status `ROT` updates |  |  |
| T24 | In normal mode ACTION short press | Toggles freeze (`LIVE` <-> `FROZEN`) |  |  |
| T25 | In normal mode ACTION long press (~1.2s) | Triggers `AXIS` cycle |  |  |
| T26 | In normal mode ACTION very long press (~2.2s) | Toggles orientation mode |  |  |
| T27 | In normal mode ACTION ultra long press (~3.2s) | Starts guided RECAL workflow |  |  |
| T28 | In guided ZERO use ACTION short press | Acts as `CONFIRM` |  |  |
| T29 | In guided ZERO use ACTION long press | Acts as `CANCEL` |  |  |
| T30 | In guided RECAL use ACTION short press | Acts as `CONFIRM` |  |  |
| T31 | In guided RECAL use ACTION long press | Acts as `CANCEL` |  |  |
| T32 | Web UI press `ZERO` | Starts guided ZERO on device; web shows `CONFIRM/CANCEL` and progress after confirm |  |  |
| T33 | Web UI press `RECAL` | Starts guided RECAL on device; web shows `CONFIRM/CANCEL` and progress after confirm |  |  |
| T34 | Tap readout area | Freeze toggles immediately; displayed values hold instantly (no lag) |  |  |
| T35 | In serial monitor send `d` twice | Raw IMU stream toggles ON then OFF |  |  |
| T36 | Power cycle after orientation/rotation/alignment/recal/zero changes | Persisted settings restore correctly from EEPROM for current mode |  |  |

## Optional Deep Check (Serial Alignment Flow)

1. Send `C` in serial monitor.
2. Follow prompts and press `c` at each requested orientation.
3. Confirm `Alignment complete` prints.
4. Complete final recalibration step (`ALIGN 7/7`) and verify return to normal operation.

Expected:

- No lockup during prompts.
- Alignment values persist after power cycle.

## Exit Criteria for Baseline Complete

- T01-T36 pass.
- Any failures are captured with reproducible steps and serial output snippets.
