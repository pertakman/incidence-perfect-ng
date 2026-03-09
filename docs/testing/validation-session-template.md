# Validation Session Template

Use this file as a template for each new validation session.

Workflow:

1. Copy this file to `docs/testing/session-logs/YYYY-MM-DD-validation-session.md`.
2. Fill in the sections below.
3. Keep one file per test session to preserve history cleanly.

## Build Identity

- Date:
- Firmware version:
- Firmware commit:
- Board:
- Tester:

## Environment

- Upload method:
- Serial port:
- Power source:
- Extra setup notes:

## Scope

- Checklist used: `docs/testing/hardware-validation-checklist.md`
- Step-specific checklist used (if any): `docs/testing/touch-ui-step2-test-suite.md`
- Tests executed:
- Optional deep check executed (`Yes`/`No`):
- STA/hostname coverage executed (`Yes`/`No`):
- OTA upload coverage executed (`Yes`/`No`):
- OTA safety-gate coverage executed (`Yes`/`No`):
- Network recovery coverage executed (`Yes`/`No`):
- Battery telemetry/charging coverage executed (`Yes`/`No`):
- Deep-sleep wake coverage executed (`Yes`/`No`):
- Wake splash/touch regression checks executed (`Yes`/`No`):

## Results Summary

- Passed:
- Failed:
- Blocked:

## Failures / Anomalies

For each issue, add one entry:

1. Test ID:
   Steps:
   Expected:
   Actual:
   Serial output excerpt:
   Photos/video:

## Observations / Decisions

- Record noteworthy behavior confirmed during testing.
- Record decisions taken during/after the session.
- Record follow-up actions if needed.

## Sign-off

- Baseline status:
- Next action:

## Attachments

- Serial logs:
- Media links:
- Related commit(s):
- OTA artifact used (`.bin` name/version):
