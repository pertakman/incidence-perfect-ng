# Beta Tester Handoff Note (Rikard)

## Build Identity

- Firmware candidate: `2026.2.16`
- Board: ESP32-S3 AMOLED 1.91 inch (`240x536`)
- Prepared date: `2026-02-28`
- Connectivity mode for this beta: Wi-Fi AP (`http://192.168.4.1`)

## Objective For This Beta Session

Collect real-world usability and reliability feedback across touch UI, ACTION button control, and phone web UI.

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
5. Reboot/power-cycle persistence for mode, rotation, align refs, and zero/offset-cal behavior.

## Quick Controls Reference

- Touch: `ZERO`, `AXIS`, `MODE`, `ALIGN`, `ROTATE`
- ACTION button:
  - normal: short = freeze, long = axis, very long = mode, ultra long = OFFSET CAL
  - guided ZERO/OFFSET CAL: short = confirm, long = cancel
  - ALIGN: short = capture
- Serial (optional): `z`, `a`, `m`, `u`, `v`, `C`, `o`, `c`, `y`, `p`, `x`

## Known Notes To Keep In Mind

- Roll conditioning is intentionally reduced near high pitch angles (around `|pitch| > 80 deg`).
- `ALIGN` and `OFFSET CAL` are separate workflows.
- If both `SCREEN UP` and `SCREEN VERTICAL` are used in practice, run OFFSET CAL in both modes.

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
- Beta release checklist: `docs/release/beta-checklist.md`
