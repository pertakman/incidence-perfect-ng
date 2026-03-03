# Validation Session Log

## Build Identity

- Date: 2026-03-03
- Firmware version observed: `2026.3.13`
- Firmware commit: `01a3cde`
- Board: ESP32-S3 1.91in AMOLED dev board (`240x536`)
- Tester: Per Takman

## Environment

- Upload method: USB (PlatformIO)
- Serial port: USB CDC @ 115200
- Power source: USB (with and without battery installed depending on test)
- Extra setup notes: verification run on two separate units

## Scope

- Checklist used: `docs/testing/hardware-validation-checklist.md`
- Tests executed: battery presence configuration + UI behavior verification
- Optional deep check executed (`Yes`/`No`): No
- STA/hostname coverage executed (`Yes`/`No`): No
- OTA upload coverage executed (`Yes`/`No`): No
- OTA safety-gate coverage executed (`Yes`/`No`): No
- Network recovery coverage executed (`Yes`/`No`): No
- Battery telemetry/charging coverage executed (`Yes`/`No`): Yes
- Deep-sleep wake coverage executed (`Yes`/`No`): No
- Wake splash/touch regression checks executed (`Yes`/`No`): No

## Results Summary

- Passed: `T55`, `T56`, `T57`
- Failed: None
- Blocked: None

## Observations / Decisions

- `Battery = No battery installed` provides clearer UX for non-battery deployments.
- In this mode, battery indicators are intentionally hidden in both web and touch UI.
- Verification was repeated on two separate units with expected behavior on both.

## Sign-off

- Baseline status: Battery presence configuration behavior verified (`T55-T57` passed).
- Next action: Publish beta release notes and continue with next feature tranche.

## Attachments

- Related commit(s): `01a3cde`
- OTA artifact used (`.bin` name/version): `.pio/build/esp32s3/firmware.bin` (`2026.3.13`)
