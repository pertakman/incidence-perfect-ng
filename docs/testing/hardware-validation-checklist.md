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

- None currently tracked as blocking for beta.

## Test Cases

| ID | Action | Expected Result | Pass/Fail | Notes |
|---|---|---|---|---|
| T01 | Power cycle board | UI boots, shows roll/pitch readout, touch responsive |  |  |
| T02 | Observe serial startup | Mode banner prints with orientation and sign conventions |  |  |
| T03 | Tilt right edge down slowly | `ROLL` moves positive, smooth update, color warning after ~30 deg |  |  |
| T04 | Tilt left edge down slowly | `ROLL` moves negative |  |  |
| T05 | Tilt top toward user slowly | `PITCH` moves positive |  |  |
| T06 | Tap `ZERO` on stable surface | Display values recenter near `+0.00 deg`; brief `ZERO APPLIED` hint is shown |  |  |
| T07 | Tap `AXIS` repeatedly | Layout cycles: both -> roll-only -> pitch-only -> both |  |  |
| T08 | Tap `ALIGN`, then `CANCEL` (`ZERO` label in align mode) | Enters alignment modal, then returns to normal UI |  |  |
| T09 | Tap `ALIGN`, capture 1-2 steps, then `CANCEL` | Step text updates clearly and flow can be aborted safely |  |  |
| T10 | Tap `MODE` once | Enters MODE workflow (`ZERO`->`CANCEL`, `MODE`->`CONFIRM`); hint shows `Position with SCREEN ...` |  |  |
| T11 | In MODE workflow press `CANCEL` | Exits MODE workflow without applying orientation change and without `ZERO APPLIED` message |  |  |
| T12 | In serial monitor send `z` | Zero reference applied, readings recenter |  |  |
| T13 | In serial monitor send `u` | Starts guided MODE workflow targeting `SCREEN UP` (does not apply immediately) |  |  |
| T14 | In serial monitor send `v` | Starts guided MODE workflow targeting `SCREEN VERTICAL` (does not apply immediately) |  |  |
| T15 | In serial monitor send `a` | Output cycles `Roll+Pitch` -> `Roll only` -> `Pitch only` |  |  |
| T16 | In serial monitor send `c` while stationary | Recalibration runs; readings settle after completion |  |  |
| T17 | Tap `ROTATE` | Full UI rotates 180 deg and status `ROT` updates |  |  |
| T18 | In MODE workflow press `CONFIRM` and hold still | Countdown appears and orientation auto-applies at end of timer |  |  |
| T19 | In MODE workflow use BOOT short press | Acts as `CONFIRM` |  |  |
| T20 | In MODE workflow use BOOT long press | Acts as `CANCEL` |  |  |
| T21 | In normal mode BOOT short press | Toggles freeze (`LIVE` <-> `FROZEN`) |  |  |
| T22 | In normal mode BOOT long press (~1.2s) | Triggers `AXIS` cycle |  |  |
| T23 | In normal mode BOOT very long press (~2.2s) | Starts/toggles MODE orientation workflow |  |  |
| T24 | Tap readout area | Freeze toggles immediately; displayed values hold instantly (no lag) |  |  |
| T25 | Touch-start MODE and monitor serial | Serial prompt reflects same target/pending state |  |  |
| T26 | Serial-start MODE (`m`/`u`/`v`) and observe screen | Screen enters MODE workflow with matching target text |  |  |
| T27 | In serial MODE workflow send `c` then hold still | Orientation auto-applies after timer |  |  |
| T28 | In serial MODE workflow send `x` | Workflow cancels and screen returns to normal |  |  |
| T29 | Power cycle after orientation/rotation/alignment changes | Persisted settings restore correctly from EEPROM |  |  |

## Optional Deep Check (Serial Alignment Flow)

Only if needed now. This is the full serial-guided routine behind `C`.

1. Send `C` in serial monitor.
2. Follow prompts and press `c` at each requested orientation.
3. Confirm `Alignment complete` prints.

Expected:

- No lockup during prompts.
- Alignment values persist after power cycle.

## Exit Criteria for Baseline Complete

- T01-T29 pass.
- Any failures are captured with reproducible steps and serial output snippets.
