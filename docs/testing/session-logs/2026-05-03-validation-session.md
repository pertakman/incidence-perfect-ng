# Validation Session - 2026-05-03

## Build Identity

- Date: 2026-05-03
- Firmware version: 2026.5.1
- Firmware commit: 88bd158 (`HEAD` on 2026-05-03; `feat: improve displacement presets`)
- Board: ESP32-S3 AMOLED 1.91 inch (`240x536`)
- Tester: Per Takman

## Environment

- Upload method: USB
- Serial port: COM3
- Power source: USB
- Extra setup notes: Full release-gate validation pass for promoting current local `HEAD` to final release `2026.5.1`.

## Scope

- Checklist used: `docs/testing/hardware-validation-checklist.md`
- Step-specific checklist used (if any): `docs/testing/touch-ui-step2-test-suite.md`
- Tests executed: Full hardware validation checklist (`T01-T69`) plus touch UI Step 2 regression suite on current local `HEAD`
- Optional deep check executed (`Yes`/`No`): Yes
- STA/hostname coverage executed (`Yes`/`No`): Yes
- OTA upload coverage executed (`Yes`/`No`): Yes
- OTA safety-gate coverage executed (`Yes`/`No`): Yes
- Network recovery coverage executed (`Yes`/`No`): Yes
- Battery telemetry/charging coverage executed (`Yes`/`No`): Yes
- Deep-sleep wake coverage executed (`Yes`/`No`): Yes
- Wake splash/touch regression checks executed (`Yes`/`No`): Yes
- Web stability/long-session coverage executed (`Yes`/`No`): Yes
- Web setup-helper coverage executed (`Yes`/`No`): Yes
  - Surface Displacement: Yes
  - Linearity: Yes
- Device settings round-trip coverage executed (`Yes`/`No`): Yes
- Manual/docs updated as part of this session (`Yes`/`No`): Yes

## Results Summary

- Passed: Full hardware validation checklist `T01-T69` passed on firmware `2026.5.1`. Touch UI Step 2 regression suite also passed without issues.
- Failed: One web diagnostics regression was found during validation and fixed in the current release build; no remaining failures at sign-off.
- Blocked: None.
- Deferred: None.

## Failures / Anomalies

For each issue, add one entry:

1. Test ID: T39
   Steps: Open web UI Diagnostics panel during validation of firmware `2026.5.1`.
   Expected: Diagnostics values update live from `/api/state`.
   Actual: Diagnostics initially failed to update; issue traced and fixed during validation. Final release candidate under test passed T39 after rebuild/reflash.
   Serial output excerpt:
   Photos/video: Browser screenshots captured during investigation.

2. No remaining open anomalies in the release-intended build after applying the diagnostics fix and rerunning the affected checks.

## Observations / Decisions

- Current local `HEAD` auto-versioned to `2026.5.1` and was used as the release-intended build.
- Full hardware validation checklist `T01-T69` completed successfully on the current build.
- Touch UI Step 2 regression suite completed successfully with no issues found.
- Web diagnostics refresh regression was discovered during T39, fixed during validation, rebuilt, reflashed, and confirmed working in the final tested build.
- Decision: Promote the validated `2026.5.1` build to final release prep, including release notes and manual PDF publication.

## Sign-off

- Baseline status: Release gate passed on firmware `2026.5.1`.
- Next action: Build/publish release manual PDF, prepare release notes/artifact manifest, and tag final release `v2026.5.1`.
- RC recommendation:
  - Ready for internal use
  - Ready for final release promotion (template is RC-oriented; this session is being used as final release validation)

## Attachments

- Serial logs:
- Media links:
- Related commit(s): `88bd158`
- OTA artifact used (`.bin` name/version): `.pio/build/esp32s3/firmware.bin` / `2026.5.1`
- Manual PDF generated: `docs/manual/releases/Incidence-Perfect-NG-Manual-2026.5.1.pdf`
