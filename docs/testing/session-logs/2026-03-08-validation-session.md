# Validation Session - 2026-03-08

## Build Identity

- Date: 2026-03-08
- Firmware version: 2026.3.16 (verify on splash)
- Firmware commit: 176edd9 (+ local uncommitted Step 2 changes)
- Board: ESP32-S3-DevKitC-1-N8
- Tester: Per Takman

## Environment

- Upload method: USB
- Serial port: COM3
- Power source: USB
- Extra setup notes: Running Step 2 touch UI release testing (Simple/Advanced + VIEW semantics)

## Scope

- Checklist used: docs/testing/hardware-validation-checklist.md
- Step-specific checklist used (if any): docs/testing/session-logs/2026-03-08-touch-ui-step2-test-suite.md
- Tests executed: Touch UI Step 2 suite (sections A-H)
- Optional deep check executed (`Yes`/`No`): Yes
- STA/hostname coverage executed (`Yes`/`No`): Yes
- OTA upload coverage executed (`Yes`/`No`): Yes
- OTA safety-gate coverage executed (`Yes`/`No`): Yes
- Network recovery coverage executed (`Yes`/`No`): Yes
- Battery telemetry/charging coverage executed (`Yes`/`No`): No
- Deep-sleep wake coverage executed (`Yes`/`No`): Yes
- Wake splash/touch regression checks executed (`Yes`/`No`): Yes

## Results Summary

- Passed: Touch UI Step 2 suite sections A-H (after retest updates)
- Failed: None open
- Blocked: None

## Failures / Anomalies

For each issue, add one entry:

General observations:

1. Closed (retest 2026-03-09): WEB UI filtering now matches touch UI display behavior closely enough for parity.

2. Closed (retest 2026-03-09): ZERO timing harmonized across touch/serial/web; stillness + averaging behavior validated.

3. Closed (retest 2026-03-09): Web button order corrected to `FREEZE | ZERO | OFFSET CAL | AXIS | MODE | ALIGN | ROTATE | SLEEP`.

## Observations / Decisions

- Record noteworthy behavior confirmed during testing.
- Record decisions taken during/after the session.
- Record follow-up actions if needed.

- Retest confirmed previously reported Step 2 findings are closed (`F2`, `F5`, `F6`).
- RC decision: Pass.

## Sign-off

- Baseline status: RC decision = Pass
- Next action: Prepare release candidate handoff package and OTA artifact communication.

## Attachments

- Serial logs:
- Media links:
- Related commit(s):
- OTA artifact used (`.bin` name/version):
