# Validation Session Log

Date: 2026-02-25
Firmware commit: f8d4586 (plus local uncommitted fixes for T02/T08/T09/T14)
Board: ESP32-S3 1.91in AMOLED dev board (240x536)
Tester: Per Takman

## Environment

- Upload method: VS Code + PlatformIO (`env: esp32s3`)
- Serial port: COM3
- Power source: USB

## Results Summary

- Passed (initial run): T01, T03, T04, T05, T06, T07, T10, T11, T12, T13, T15, T16, T17
- Failed (initial run): T02, T08, T09, T14
- Blocked: None

## Failures / Anomalies

1. Test ID: T02
   Steps: Observe startup serial output for orientation/sign convention banner.
   Expected: Startup prints include orientation and sign convention details.
   Actual: Could not capture startup behavior/prints reliably.
   Serial output excerpt: Not captured.
   Photos/video: N/A

2. Test ID: T08, T09
   Steps: Enter ALIGN flow and read instruction text.
   Expected: Instruction text should be clearly readable.
   Actual: Instruction text displayed on top of other text, making it hard/impossible to read.
   Serial output excerpt: N/A (UI issue).
   Photos/video: N/A

3. Test ID: T14
   Steps: Send `v` command to switch to vertical orientation.
   Expected: Orientation should also be reflected in top status text (e.g. `SCREEN VERTICAL | BOTH`).
   Actual: Orientation works functionally, but top status text remains `SCREEN UP | BOTH` and does not reflect dynamic state.
   Serial output excerpt: Orientation command accepted and works.
   Photos/video: N/A

## Follow-up Changes Applied

- Added dynamic top status label update (`SCREEN UP/SCREEN VERTICAL | BOTH/ROLL/PITCH`).
- Improved ALIGN modal readability by hiding top status in modal states and adding opaque background/padding for instruction panel.
- Added serial attach transition print logic (removed duplicate triple-print behavior).
- Build verification: PASS (`pio run -e esp32s3`).

## Re-test Update (User Confirmed)

- Closed after re-test:
  - `T02` startup serial visibility
  - `T08` ALIGN modal readability
  - `T09` ALIGN modal readability/flow text
  - `T12/T13` serial command behavior

## Sign-off

- Baseline status: IN PROGRESS
- Next action: Re-run against current checklist IDs T01-T39 (guided ZERO/OFFSET CAL, immediate MODE, ACTION-button roles, freeze immediacy, and touch/serial/web sync).

## Additional Test Coverage To Include (Current UI/FW)

- Guided ZERO/OFFSET CAL workflow parity:
  - touch-started workflow must appear in serial/web state
  - serial/web-started workflow must appear on-screen
  - `CONFIRM`/`CANCEL` parity across touch, ACTION button, serial (`c`/`x`), and web controls
  - hold-still countdown resets on movement
  - workflow applies automatically when timer + sampling complete
- MODE behavior:
  - touch/serial/web/ACTION mode changes apply immediately
  - mode-specific persisted calibration/zero references are loaded correctly
- ACTION-button role matrix:
  - Normal mode: short/long/very-long/ultra-long actions
  - ZERO workflow: short=confirm, long=cancel
  - OFFSET CAL workflow: short=confirm, long=cancel
  - ALIGN workflow: capture
- Freeze behavior:
  - readout tap freezes immediately at currently displayed value (no visual lag)
  - status toggles `LIVE`/`FROZEN`
- UI readability/touchability:
  - hint lane remains outside readout area
  - button press reliability with expanded touch hit area
- Serial readability:
  - command response pauses stream until explicit resume (`Enter`/`Space`/`g`)
- Remote diagnostics:
  - diagnostics card updates continuously during normal use and workflows
