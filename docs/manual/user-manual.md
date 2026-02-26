Incidence Perfect NG is a compact 2-axis inclinometer/incidence meter built around an ESP32-S3 + AMOLED touchscreen. It measures **roll** and **pitch** and provides guided workflows for **ZERO**, **MODE**, **ROTATE**, and **ALIGN**.

> Beta note: This manual describes the current firmware behavior. If something differs on your device, report the **firmware version shown on the splash screen**.

---

## 1) Getting Started

### Power + Boot

- Connect the device over USB power.
- On boot you'll see the splash screen with the firmware version in the lower-right.
- After boot, the live readout screen appears (roll/pitch).

### What You're Looking At

- **Top status line** shows:
  - orientation mode (`SCREEN UP` or `SCREEN VERTICAL`)
  - axis view (`BOTH`, `ROLL`, `PITCH`)
  - rotation (`ROT 0` or `ROT 180`)
  - live state (`LIVE` or `FROZEN`)
- **Readouts**:
  - `ROLL` (left) and `PITCH` (right) in degrees
  - colors shift for large angles (warning, then critical)
- **Bottom buttons**:
  - `ZERO`, `AXIS`, `MODE`, `ALIGN`, `ROTATE`

---

## 2) Touch Controls (Everyday Use)

### ZERO

Use when the tool is resting in the reference position.

- Tap `ZERO`.
- A brief confirmation message appears (`Zero applied - hold still briefly`).
- A short progress bar is shown while zero is being applied.
- Values settle around `0.00`.

### AXIS

Choose what you want to focus on:

- Tap `AXIS` to cycle: `BOTH -> ROLL -> PITCH -> BOTH`.

### ROTATE (180 degrees)

Use when the device is physically hard to read and you want to flip the UI.

- Tap `ROTATE` to toggle `ROT 0` / `ROT 180`.
- Rotation persists after reboot.

### Freeze / Unfreeze

Use freeze when you want to capture a reading without chasing tiny motion.

- Tap the **readout area** (the roll/pitch values) to toggle `LIVE` / `FROZEN`.
- When frozen, the displayed values hold steady.

---

## 3) ACTION Button (Physical Control)

<div class="action-button-intro">
  <p>The board has a physical <strong>ACTION button</strong> (electrically <code>GPIO0</code>, labeled <code>BOOT/GPO</code> on the board). It mirrors key actions so you can operate the device when the screen is hard to reach.</p>
  <p><img src="../assets/action-button-reference.png" alt="ACTION button reference"/></p>
</div>

In normal measurement mode:

- **Short press**: toggle freeze (`LIVE` <-> `FROZEN`)
- **Long press (~1.2s)**: cycle `AXIS` (`BOTH -> ROLL -> PITCH`)
- **Very long press (~2.2s)**: enter/toggle `MODE` workflow (orientation change)

While holding the ACTION button, an on-screen hint shows what will happen on release and a progress indicator for the next threshold.
Countdowns are shown as `X.X s` (with a space before `s`).

---

## 4) MODE (Orientation Change, Guided)

`MODE` changes how the device interprets orientation.

- `SCREEN UP`: standard "screen facing up" use case
- `SCREEN VERTICAL`: use case where the tool is used on a vertical surface

### Touch Workflow

1. Tap `MODE` once to enter the guided workflow.
2. The UI shows the required repositioning target:
   - `Reposition with screen facing up`, or
   - `Reposition with screen vertically`
3. Tap `CONFIRM`, then **hold the device still**.
4. A hold timer appears as `Hold still X.X s`, with a progress bar.
5. Near completion, the UI switches to `Applying...` and then auto-applies the new mode.
6. Tap `CANCEL` at any time to abort without changes.

### ACTION Button in MODE Workflow

- Short press: `CONFIRM`
- Long press: `CANCEL`

---

## 5) ALIGN (Mechanical Alignment, 6 Steps)

Use `ALIGN` after mounting/enclosing the device to remove systematic bias.

This is a guided, 6-orientation capture procedure. The device will prompt you through the positions and ask you to capture each one.

### Touch Workflow

1. Tap `ALIGN`.
2. Follow the on-screen instruction (example: `Place tool: SCREEN UP`).
3. Tap `CAPTURE` to record that step.
4. Repeat until all steps are captured.
5. Tap `CANCEL` to abort safely at any time.

### ACTION Button in ALIGN Workflow

If the screen is hard to access (for example screen-down steps):

- ACTION button short press = `CAPTURE`

---

## 6) Serial Control (Optional)

If connected to a PC, you can control the same workflows via serial (115200).

Core commands:

- `z`: ZERO now
- `a`: AXIS cycle (`BOTH -> ROLL -> PITCH`)
- `r`: ROTATE 180 toggle
- `C`: start ALIGN workflow
- `c`: confirm/capture (context-sensitive)
- `m`: start MODE workflow to the opposite orientation
- `u`: start MODE workflow targeting `SCREEN UP`
- `v`: start MODE workflow targeting `SCREEN VERTICAL`
- `x`: cancel pending MODE workflow

Serial and touch workflows are designed to stay synchronized.

---

## 7) Troubleshooting

### Touch Feels Hard To Trigger

- Use ACTION button alternatives for critical actions.
- Try deliberate taps (not swipes) centered on the button.

### Serial Monitor Doesn't Resume After Reset

- Some setups require closing/reopening the serial monitor after reset.
- Always report the firmware version shown on splash if you see inconsistent serial behavior.

### MODE Doesn't Apply

- Ensure you press `CONFIRM`, then keep the device still until `Applying...` appears and the mode change completes.
- If you move, the countdown may reset (by design).

---

## 8) Beta Tester Checklist + Feedback

If you're testing externally, use:

- `../release/beta-checklist.md`
- `../release/tester-handoff-note.md`

When reporting an issue, include:

1. Firmware version (from splash)
2. Exact steps to reproduce
3. Expected vs actual result
4. Photos/video if UI-related

---

## Appendix A: Hardware Notes

For reference bring-up settings (Arduino IDE), see:

- `../hardware/board-settings.md`
- `../hardware/board-settings-arduino-ide.jpg`

---

## Authorship

This firmware and UI were developed by **Per Takman**, with assistance from **OpenAI Codex (Codex CLI)**.

---

## License And Warranty

- The project source code is released under the MIT License.
- The software is provided "AS IS", without warranty of any kind.
- Third-party manuals, schematics, and library dependencies remain under their respective original licenses/terms.


