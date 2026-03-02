# Validation Session Log

## Build Identity

- Date: 2026-03-02
- Firmware version observed in web header: `2026.3.9`
- Board: ESP32-S3 1.91in AMOLED dev board (`240x536`)
- Tester: Per Takman

## Environment

- Upload method: USB (PlatformIO) + OTA (web UI)
- Power source: USB
- Control path: device touch UI + phone/web UI

## Scope

- Checklist used: `docs/testing/hardware-validation-checklist.md`
- Tests executed: targeted network/OTA hardening (`T50-T53`)
- Optional deep check executed: No
- STA/hostname coverage executed: Not in this session
- OTA upload coverage executed: Yes
- OTA safety-gate coverage executed: Yes
- Network recovery coverage executed: Yes

## Results Summary

- Passed: `T50`, `T51`, `T52`, `T53`
- Failed: None
- Blocked: None

## Notes / Evidence

- Browser SHA-256 auto-calc was unavailable on `http://192.168.4.1` (insecure origin).
  - SHA-256 was computed via PowerShell and pasted into the OTA panel.

## Test Details

### T50 Version Gate Reject (No Reboot Expected)

- Action:
  - Select `firmware.bin`
  - Set target version = current (`2026.3.9`)
  - Paste correct SHA-256 for selected file
  - Ensure `Allow reinstall/downgrade` is unchecked
- Expected:
  - OTA rejected by version gate, no reboot, FW remains unchanged
- Actual:
  - `OTA failed: version gate rejected: target 2026.3.9 is not newer than current 2026.3.9`
  - No reboot observed; FW remained `2026.3.9`

### T51 Checksum Gate Reject (No Reboot Expected)

- Action:
  - Select `firmware.bin`
  - Set target version to a higher value (`2026.3.10`)
  - Paste an intentionally incorrect SHA-256 (one character changed)
- Expected:
  - OTA rejected by checksum gate, no reboot, FW remains unchanged
- Actual:
  - `OTA failed: sha256 mismatch`
  - FW remained `2026.3.9`

### T52 Web Network Recovery

- Action:
  - Use web `Network` panel -> `Recover AP Mode`
- Expected:
  - Device forces AP-only and clears saved STA credentials; AP remains reachable
- Actual:
  - Works as expected

### T53 Physical Network Recovery

- Action:
  - Reboot device
  - Immediately press+hold ACTION continuously through startup for ~2 s
- Expected:
  - Device forces AP-only and clears saved STA credentials; AP remains reachable
- Actual:
  - Works as expected

## Sign-off

- Baseline status: Targeted hardening checks COMPLETE (`T50-T53` passed on 2026-03-02)
- Next action: Produce a pinned beta artifact + SHA-256 and publish as a beta release candidate

