# Validation Session - 2026-03-15

## Build Identity

- Date: 2026-03-15
- Firmware version: 2026.3.22-rc3
- Firmware commit: (tag commit)
- Board: ESP32-S3 AMOLED 1.91 inch
- Tester: Per Takman

## Environment

- Upload method: USB
- Serial port: not recorded
- Power source: USB
- Extra setup notes: Targeted regression validation for Rikard feedback fixes and final RC3 publish prep.

## Scope

- Checklist used: docs/testing/hardware-validation-checklist.md
- Step-specific checklist used (if any): none
- Tests executed: targeted boot/regression checks for splash rotation, startup ZERO enable/disable persistence, and web `Network` panel wording/settings save flow
- Optional deep check executed (`Yes`/`No`): No
- STA/hostname coverage executed (`Yes`/`No`): No
- OTA upload coverage executed (`Yes`/`No`): No
- OTA safety-gate coverage executed (`Yes`/`No`): No
- Network recovery coverage executed (`Yes`/`No`): No
- Battery telemetry/charging coverage executed (`Yes`/`No`): No
- Deep-sleep wake coverage executed (`Yes`/`No`): No
- Wake splash/touch regression checks executed (`Yes`/`No`): Yes

## Results Summary

- Passed: Splash follows stored rotation; startup ZERO behaves correctly in both enabled/disabled modes; startup-ZERO setting persists across reboot; `Network` panel save flow remains functional.
- Failed: None observed in this targeted session.
- Blocked: Full OTA retest deferred to external RC3 pass.

## Failures / Anomalies

For each issue, add one entry:

- None in this session.

## Observations / Decisions

- Rikard feedback items are addressed in the current RC3 candidate.
- `Startup ZERO` remains enabled by default, but the operator can now disable it from the web UI when preserving the saved zero reference is preferable.
- RC decision: prepare `2026.3.22-rc3` for publication and external validation.

## Sign-off

- Baseline status: RC3 candidate approved for publish prep
- Next action: publish RC3 firmware package, release note, and manual PDF

## Attachments

- Serial logs:
- Media links:
- Related commit(s):
- OTA artifact used (`.bin` name/version): `.pio/build/esp32s3/firmware.bin` / `2026.3.22-rc3`
