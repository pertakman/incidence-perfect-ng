# Validation Session Log

Date: 2026-02-25
Firmware commit: f8d4586 (plus local uncommitted fixes for T02/T08/T09/T14)
Board: ESP32-S3 1.91in AMOLED dev board (240x536)
Tester: User

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
- Next action: Confirm remaining serial/UI status behavior and then finalize baseline sign-off.
