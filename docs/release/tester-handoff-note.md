# Beta Tester Handoff Note (Rikard)

## Build Identity

- Firmware candidate: `2026.3.22-rc3`
- Release tag: `v2026.3.22-rc3`
- Board: ESP32-S3 AMOLED 1.91 inch (`240x536`)
- Prepared date: `2026-03-15`
- Connectivity mode for this beta: Wi-Fi AP (`http://192.168.4.1`)
- OTA target version for the provided RC3 artifact: `2026.3.22`

## Objective For This Beta Session

Confirm the boot-time polish changes landed cleanly: splash orientation now follows stored rotation, startup ZERO can run automatically on cold boot, and the startup-ZERO setting persists and behaves correctly from the web UI.

## What To Test

1. Everyday flow: `ZERO`, `AXIS`, `MODE`, `ROTATE`, freeze/unfreeze.
2. Guided workflows: `OFFSET CAL` and full 6-step `ALIGN`.
3. ACTION button behavior:
   - short, long, very long, and ultra long holds in normal mode
   - `CONFIRM`/`CANCEL` behavior in guided workflows
   - `CAPTURE` during ALIGN
4. Phone web UI parity with device UI:
   - controls appear only when relevant
   - progress bars shown during hold/sampling phases
   - diagnostics panel updates continuously without disconnect
5. Boot behavior:
   - splash orientation matches stored `ROT 0` / `ROT 180`
   - `Startup ZERO = Enabled` starts ZERO automatically on cold boot
   - `Startup ZERO = Disabled` leaves saved zero reference untouched on cold boot
6. Reboot/power-cycle persistence for mode, rotation, startup-ZERO setting, align refs, and zero/offset-cal behavior.
7. OTA behavior for RC3:
   - valid newer upload installs cleanly
   - wrong SHA-256 is rejected
   - same-version upload without `Force` is rejected

## Quick Controls Reference

- Touch: `ZERO`, `AXIS`, `MODE`, `ALIGN`, `ROTATE`
- ACTION button:
  - normal: short = freeze, long = axis, very long = mode, ultra long = OFFSET CAL
  - guided ZERO/OFFSET CAL: short = confirm, long = cancel
  - ALIGN: short = capture
- Serial (optional): `z`, `a`, `m`, `u`, `v`, `C`, `o`, `c`, `y`, `p`, `x`

## OTA Quick Steps (Web UI)

1. Connect to device AP and open `http://192.168.4.1`.
2. Expand `OTA Update`.
3. Select the provided release-candidate `.bin` (ESP32-S3 build).
4. Enter the target firmware version exactly (`2026.3.22` for the supplied RC3 file).
5. Enter matching SHA-256 (from release note or local hash command).
6. Keep `Force` unchecked for normal update.
7. Tap `Upload & Install`, wait for reboot, reconnect, and confirm splash version.

Expected safety-gate behavior:

- Same-version upload without `Force` must be rejected.
- Wrong SHA-256 must be rejected.
- On reject, current firmware stays running.

## Known Notes To Keep In Mind

- Roll conditioning is intentionally reduced near high pitch angles (around `|pitch| > 80 deg`).
- `ALIGN` and `OFFSET CAL` are separate workflows.
- If both `SCREEN UP` and `SCREEN VERTICAL` are used in practice, run OFFSET CAL in both modes.
- `Startup ZERO` applies on cold boot; it does not re-run automatically after deep-sleep wake.

## Feedback Format Requested

Please report each issue with:

1. Firmware version shown on splash screen.
2. Exact action sequence.
3. Expected result vs actual result.
4. Reproducibility (`always` / `sometimes` / `once`).
5. Photo/video for UI-related issues.

## Test Documents

- Main checklist: `docs/testing/hardware-validation-checklist.md`
- Session log template: `docs/testing/validation-session-template.md`
- Session logs (history): `docs/testing/session-logs/`
- Beta release checklist: `docs/release/beta-checklist.md`
