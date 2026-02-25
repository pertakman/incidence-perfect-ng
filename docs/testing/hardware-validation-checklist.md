# Hardware Validation Checklist

Use this checklist against the current firmware build to establish a known-good baseline.

## Current Session Status (2026-02-25)

- Closed: `T02`, `T08`, `T09`, `T12`, `T13`
- Remaining for final baseline sign-off: confirm all other tests remain stable after latest firmware upload.

## Preconditions

- Firmware uploaded from PlatformIO (`env: esp32s3`).
- Board powered over USB and responsive.
- Serial Monitor open at `115200`.
- Device resting on a stable, mostly level surface.

## Known Current UI Limitations

- `ALIGN` in UI runs a temporary capture path (`alignmentCapture`) and does not execute full 6-orientation alignment flow.

## Test Cases

| ID | Action | Expected Result | Pass/Fail | Notes |
|---|---|---|---|---|
| T01 | Power cycle board | UI boots, shows roll/pitch readout, touch responsive |  |  |
| T02 | Observe serial startup | Mode banner prints with orientation and sign conventions |  |  |
| T03 | Tilt right edge down slowly | `ROLL` moves positive, smooth update, color warning after ~30 deg |  |  |
| T04 | Tilt left edge down slowly | `ROLL` moves negative |  |  |
| T05 | Tilt top toward user slowly | `PITCH` moves positive |  |  |
| T06 | Tap `ZERO` on stable surface | Display values recenter near `+0.00 deg` |  |  |
| T07 | Tap `AXIS` repeatedly | Layout cycles: both -> roll-only -> pitch-only -> both |  |  |
| T08 | Tap `ALIGN`, then `CANCEL` (`ZERO` label in align mode) | Enters alignment modal, then returns to normal UI |  |  |
| T09 | Tap `ALIGN`, then `CAPTURE`, then `DONE` | State machine transitions correctly with instruction text updates |  |  |
| T10 | Tap `MODE` | Toggle orientation mode (`SCREEN UP` <-> `SCREEN VERTICAL`) |  |  |
| T11 | Tap `ROTATE` | Full UI rotates 180 deg and status `ROT` updates |  |  |
| T12 | In serial monitor send `z` | Zero reference applied, readings recenter |  |  |
| T13 | In serial monitor send `u` | Orientation set to screen-up, mode text prints in serial |  |  |
| T14 | In serial monitor send `v` | Orientation set to screen-vertical, mode text prints in serial |  |  |
| T15 | In serial monitor send `a` | Output cycles `Roll+Pitch` -> `Roll only` -> `Pitch only` |  |  |
| T16 | In serial monitor send `c` while stationary | Recalibration runs; readings settle after completion |  |  |
| T17 | Power cycle after T13/T15 changes | Orientation/incidence preferences persist from EEPROM |  |  |

## Optional Deep Check (Serial Alignment Flow)

Only if needed now. This is the full serial-only routine behind `A`.

1. Send `A` in serial monitor.
2. Follow prompts and press `c` at each requested orientation.
3. Confirm `Alignment complete` prints.

Expected:

- No lockup during prompts.
- Alignment values persist after power cycle.

## Exit Criteria for Baseline Complete

- T01-T09 and T12-T17 pass.
- Any failures are captured with reproducible steps and serial output snippets.
