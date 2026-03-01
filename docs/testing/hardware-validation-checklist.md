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
| T09 | Long-press `ZERO` in normal mode | Enters guided OFFSET CAL (`ZERO`->`CANCEL`, `MODE`->`CONFIRM`) |  |  |
| T10 | In guided OFFSET CAL press `CONFIRM` and hold still | OFFSET CAL completes with progress bar; readings settle |  |  |
| T11 | Tap `AXIS` repeatedly | Layout cycles: both -> roll-only -> pitch-only -> both |  |  |
| T12 | Tap `ALIGN`, then `CANCEL` | Enters alignment modal, then returns to normal UI |  |  |
| T13 | Tap `ALIGN`, capture 1-2 steps, then `CANCEL` | Step text updates clearly and flow can be aborted safely |  |  |
| T14 | Tap `MODE` once | Orientation toggles immediately (`SCREEN UP` <-> `SCREEN VERTICAL`) |  |  |
| T15 | In serial monitor send `z` | Starts guided ZERO workflow |  |  |
| T16 | In serial monitor send `c` while ZERO pending | Confirms ZERO workflow and starts hold/sampling |  |  |
| T17 | In serial monitor send `o` | Starts guided OFFSET CAL workflow |  |  |
| T18 | In serial monitor send `c` while OFFSET CAL pending | Confirms OFFSET CAL workflow and starts hold/sampling |  |  |
| T19 | In serial monitor send `u` | Orientation switches immediately to `SCREEN UP` |  |  |
| T20 | In serial monitor send `v` | Orientation switches immediately to `SCREEN VERTICAL` |  |  |
| T21 | In serial monitor send `m` | Orientation toggles immediately |  |  |
| T22 | In serial monitor send `x` during ZERO/OFFSET CAL/ALIGN | Active workflow is canceled |  |  |
| T23 | Tap `ROTATE` | Full UI rotates 180 deg and status `ROT` updates |  |  |
| T24 | In normal mode ACTION short press | Toggles freeze (`LIVE` <-> `FROZEN`) |  |  |
| T25 | In normal mode ACTION long press (~1.2s) | Triggers `AXIS` cycle |  |  |
| T26 | In normal mode ACTION very long press (~2.2s) | Toggles orientation mode |  |  |
| T27 | In normal mode ACTION ultra long press (~3.2s) | Starts guided OFFSET CAL workflow |  |  |
| T28 | In guided ZERO use ACTION short press | Acts as `CONFIRM` |  |  |
| T29 | In guided ZERO use ACTION long press | Acts as `CANCEL` |  |  |
| T30 | In guided OFFSET CAL use ACTION short press | Acts as `CONFIRM` |  |  |
| T31 | In guided OFFSET CAL use ACTION long press | Acts as `CANCEL` |  |  |
| T32 | Web UI press `ZERO` | Starts guided ZERO on device; web shows `CONFIRM/CANCEL` and progress after confirm |  |  |
| T33 | Web UI press `OFFSET CAL` | Starts guided OFFSET CAL on device; web shows `CONFIRM/CANCEL` and progress after confirm |  |  |
| T34 | Tap readout area | Freeze toggles immediately; displayed values hold instantly (no lag) |  |  |
| T35 | In serial monitor send `d` twice | Raw IMU stream toggles ON then OFF |  |  |
| T36 | Power cycle after orientation/rotation/alignment/offset cal/zero changes | Persisted settings restore correctly from EEPROM for current mode |  |  |
| T37 | Send `h` in serial monitor | Help prints and live stream pauses |  |  |
| T38 | After T37 press `Enter` (or send `g`) | Live stream resumes (`Roll/Pitch` lines continue) |  |  |
| T39 | Open web diagnostics panel | Raw/remapped/corrected values and refs update live without UI disconnect |  |  |

## Optional Deep Check (Serial Alignment Flow)

1. Send `C` in serial monitor.
2. Follow prompts and press `c` at each requested orientation.
3. Confirm `Alignment complete` prints.
4. Verify return to normal operation immediately after the 6th capture.

Expected:

- No lockup during prompts.
- Alignment values persist after power cycle.

## Exit Criteria for Baseline Complete

- T01-T39 pass.
- Any failures are captured with reproducible steps and serial output snippets.
