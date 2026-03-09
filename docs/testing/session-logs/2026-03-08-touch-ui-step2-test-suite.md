# Touch UI Step 2 Test Suite (Simple + Advanced Layouts)

Use this checklist to validate the Step 2 touch layout behavior.

Logging reminder:

- Run this checklist together with `docs/testing/validation-session-template.md`.
- Record outcomes in a dated session log under `docs/testing/session-logs/`.

## Build Identity

- Firmware version: 2026.3.16 (verify on splash)
- Firmware commit: 176edd9 (+ local uncommitted Step 2 changes)
- Tester: Per Takman
- Date: 2026-03-08

## Preconditions

- Device boots to normal measurement screen.
- Touch screen calibrated/working.
- Battery/USB power stable during test.

## A) Advanced Mode Baseline

1. Confirm `Advanced` layout shows 5 buttons:
   - `ZERO`, `AXIS`, `MODE`, `ALIGN`, `ROTATE`
2. Tap `AXIS` cycles:
   - `BOTH -> ROLL -> PITCH -> BOTH`
3. Tap `MODE` in normal state:
   - orientation mode toggles (`UP` <-> `VERT`)
4. Confirm existing workflows still work:
   - `ZERO` confirm/cancel flow
   - `ALIGN` capture/cancel flow
   - `ROTATE` toggle

Pass/Fail: Pass

## B) Advanced -> Simple Transition

1. In normal state, press and hold `MODE`.
2. Verify tip text appears and counts down toward UI switch.
3. Hold past long-hold threshold, then release.
4. Confirm layout switches to `Simple`.

Expected:

- No web UI required.
- Transition works from touch only.

Pass/Fail: Pass

## C) Simple Mode Visual Layout

1. Confirm only 2 buttons are visible:
   - `ZERO`
   - `VIEW`
2. Confirm hidden in `Simple`:
   - `AXIS`, `ALIGN`, `ROTATE`
3. Confirm readouts/status still update normally.

Pass/Fail: Pass

## D) Simple Mode Button Semantics (`VIEW`)

1. Short tap `VIEW`:
   - cycles `BOTH -> ROLL -> PITCH -> BOTH`
2. Medium hold `VIEW` then release:
   - orientation toggles `UP` <-> `VERT`
3. Long hold `VIEW` then release:
   - layout switches back to `Advanced`

Pass/Fail: Pass

## E) Tip / Hint Behavior

1. In `Advanced`, hold `MODE`:
   - tip must indicate upcoming `SWITCH UI`
2. In `Simple`, hold `VIEW`:
   - early band: `AXIS` context
   - medium band: `MODE` context
   - long band: `SWITCH UI` context
3. Confirm countdown updates live (not static/stale).

Pass/Fail: Pass

## F) Persistence

1. Switch to `Simple`, reboot device.
2. Confirm it boots back in `Simple`.
3. Switch to `Advanced`, reboot device.
4. Confirm it boots back in `Advanced`.
5. Confirm axis selection persists as expected by current firmware policy.
6. Confirm orientation mode persists as expected by current firmware policy.

Pass/Fail: Pass

## G) Regression Sweep

1. No flicker regressions around readout and button bar.
2. Divider line above buttons remains visible.
3. `CONFIRM` and `CAPTURE` text fits inside buttons.
4. Long-press tips remain readable and do not occlude critical workflow text.
5. ACTION physical button behavior unchanged.

Pass/Fail: Pass

## H) OTA + Safety Gate Coverage

1. From web UI, upload a valid `.bin` with:
   - target version newer than current
   - matching SHA-256
2. Confirm OTA completes and device reboots.
3. Confirm reported firmware version after reboot is updated.
4. Negative gate check:
   - try same-version upload without force (must be rejected), or
   - provide mismatched SHA-256 (must be rejected).
5. Confirm device remains operational after failed gate check.

Pass/Fail: Pass

## Notes / Findings

Retest update (2026-03-09):

- F2: Closed. Boot flicker (Advanced briefly shown before Simple) no longer observed in current candidate.
- F5: Closed. Axis selection now persists across reboot as expected.
- F6: Closed. Brief `UP` flicker before `VERT` no longer observed in current candidate.
