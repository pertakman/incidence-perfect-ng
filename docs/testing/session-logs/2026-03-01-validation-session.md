# Validation Session Log

## Build Identity

- Date: 2026-03-01
- Firmware version observed in startup banner: `2026.3.3`
- Board: ESP32-S3 1.91in AMOLED dev board (`240x536`)
- Tester: Per Takman

## Environment

- Upload method: VS Code + PlatformIO (`env: esp32s3`)
- Serial port: COM3
- Power source: USB

## Scope

- Checklist used: `docs/testing/hardware-validation-checklist.md`
- Tests executed: full checklist `T01-T39`
- Optional deep check executed: Yes (serial ALIGN flow)

## Results Summary

- Passed: `T01-T39`
- Failed: None
- Blocked: None

## T02 Startup Serial Excerpt (Captured)

```text
QMI8658 Inclinometer FW 2026.3.3
ACTION button mapped to GPIO0 (BOOT, active-low)
Loaded bias offsets from EEPROM
Loaded zero reference from EEPROM

=== MODE ===
Orientation: SCREEN UP
Roll:  + = right edge down
Pitch: + = screen tilts toward you
===================

[remote] Wi-Fi AP mode
[remote] SSID: IncidencePerfectNG-85A0
[remote] PASS: incidence-ng
[remote] URL:  http://192.168.4.1
Roll: 63.90  Pitch: 58.74
Roll: 57.58  Pitch: 52.90
Roll: 51.87  Pitch: 47.65
```

## Observations / Decisions

- Serial `c` in normal mode starts and confirms `OFFSET CAL` immediately (`start+confirm`).
- Team decision: keep current behavior as an intentional serial power-user shortcut.
- Rationale: low practical risk because serial usage is limited for expected end users.

## Sign-off

- Baseline status: COMPLETE (`T01-T39` passed on 2026-03-01)
- Next action: collect external beta feedback and track any reported deviations.
