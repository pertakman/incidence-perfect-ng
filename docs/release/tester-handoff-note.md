# Beta Tester Handoff Note

## Build

- Firmware: `FW_VERSION_PLACEHOLDER`
- Board: ESP32-S3 AMOLED 1.91 inch (240x536)
- Build date: `YYYY-MM-DD`

## What To Test

1. Normal measurement flow (`ZERO`, `AXIS`, `MODE`, `ROTATE`).
2. Guided `ALIGN` workflow end-to-end.
3. ACTION button actions (short/long/very-long hold).
4. Freeze behavior and responsiveness.
5. Reboot/power-cycle persistence.

## Quick Controls

- Touch buttons: `ZERO`, `AXIS`, `MODE`, `ALIGN`, `ROTATE`
- ACTION button:
  - normal: short = freeze, long = axis, very-long = mode
  - align: press = capture
  - mode guide: short = confirm, long = cancel
- Serial:
  - `z`, `a`, `m`, `u`, `v`, `C`, `c`, `r`, `x`

## Known Issues / Notes

- Add current known limitations here before sending.
- Example: "Serial monitor may need reconnect after board reset in VS Code."

## Feedback Requested

Please report:
1. Exact firmware version shown on splash.
2. Exact action sequence that produced issue.
3. Expected result vs actual result.
4. Whether issue is reproducible (always/sometimes/once).
5. Photos/video if UI-related.
