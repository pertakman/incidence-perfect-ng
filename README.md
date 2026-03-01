# incidence-perfect-ng

2-axis incidence meter built around an ESP32-S3 1.91 inch AMOLED display development board (240x536) with onboard accelerometer/gyroscope and touch.

Board reference:
https://www.banggood.com/ESP32-S3-1_91inch-AMOLED-Display-Development-Board-240536-32-bit-LX7-Dual-core-Processor-Accelerometer-Gyroscope-Sensor-p-2024047.html?rmmds=myorder&cur_warehouse=CN&ID=6329757

## VS Code setup (PlatformIO)

This repo is now configured as a PlatformIO project for VS Code.

1. Install VS Code extension `PlatformIO IDE`.
2. Open this folder in VS Code.
3. Let PlatformIO install toolchains and libraries.
4. Build:
   - PlatformIO sidebar -> `PROJECT TASKS` -> `esp32s3` -> `Build`
   - or terminal: `pio run`
5. Upload:
   - PlatformIO sidebar -> `Upload`
   - or terminal: `pio run -t upload`
6. Monitor serial:
   - PlatformIO sidebar -> `Monitor`
   - or terminal: `pio device monitor -b 115200`

## Required Dependencies And Tooling

Firmware build/runtime dependencies:
- `PlatformIO Core` (installed via VS Code PlatformIO extension)
- `espressif32` platform (`platform = espressif32`, currently `6.12.0` in lockstep with project environment)
- `framework = arduino` for ESP32-S3
- Libraries from `platformio.ini`:
  - `lvgl/lvgl @ ^8.4.0`
  - `moononournation/GFX Library for Arduino @ 1.3.8`
  - `QMI8658` from local path: `file://C:/dev/Arduino/Libraries/QMI8658`

Important local dependency note:
- This project currently expects `QMI8658` at `C:/dev/Arduino/Libraries/QMI8658`.
- If your path differs, update `lib_deps` in `platformio.ini` accordingly.

Optional documentation/splash tooling:
- `PowerShell` (used for helper scripts in `scripts/`)
- `pandoc` (auto-managed by `scripts/ensure_pandoc.ps1` into `tools/pandoc/`)
- Headless Chrome/Edge (used by `scripts/build_manual_pdf.ps1` for PDF rendering)
- `System.Drawing` support in PowerShell/.NET (used by `scripts/generate_splash.ps1`)

## Fresh Unit Preparation

Use this before first-time flashing a new hardware unit.

1. Connect only one unit at a time.
2. Erase flash once:
   - terminal: `pio run -e esp32s3 -t erase`
3. Upload firmware:
   - terminal: `pio run -e esp32s3 -t upload`
4. Open serial monitor:
   - terminal: `pio device monitor -b 115200`
5. Run a quick sanity check:
   - `ZERO` works
   - `AXIS` cycles (`BOTH -> ROLL -> PITCH`)
   - `MODE` toggles orientation (`SCREEN UP <-> SCREEN VERTICAL`)
   - `OFFSET CAL` workflow can be started (touch long-press `ZERO`, serial `o` or `c`, web `OFFSET CAL`)
   - `ROTATE` flips 180 and persists after reboot

## Serial Commands

- `z`: start guided zero workflow (`c` to confirm, `x` to cancel)
- `c`: context action
  - ALIGN active: capture current ALIGN step
  - ZERO pending: confirm ZERO
  - OFFSET CAL pending: confirm OFFSET CAL
  - otherwise: start + confirm guided OFFSET CAL
- `y`: explicit confirm (ZERO/OFFSET CAL workflows only)
- `p`: explicit capture (ALIGN only)
- `o`: start guided OFFSET CAL workflow (confirm with `c`, cancel with `x`)
- `C`: start guided 6-step mechanical alignment workflow (shared by touch UI and serial)
- `u`: set orientation mode to `SCREEN UP` immediately
- `v`: set orientation mode to `SCREEN VERTICAL` immediately
- `m`: toggle orientation mode immediately
- `x` or `n`: cancel active workflow(s)

Touch UI, serial, ACTION button, and web share the same ZERO/OFFSET CAL/ALIGN state.
- `a`: cycle axis display/output (`BOTH -> ROLL -> PITCH`)
- `r`: toggle 180-degree screen rotation
- `d`: toggle raw IMU debug stream (5 Hz)
- `D`: print one raw IMU sample immediately
- `s`: print runtime status snapshot (mode, workflow states, offsets/references)
- `h` or `?`: print serial help
- After any serial command response, live scrolling output pauses.
  - Press `Enter`, `Space`, or send `g` to resume live stream.

## Hardware Controls

- `ACTION button` (`GPIO0`, active-low, labeled `BOOT/GPO` on board):
  - In ALIGN workflow: press/release = `CAPTURE`
  - In ZERO workflow: short press = `CONFIRM`, long press = `CANCEL`
  - In OFFSET CAL workflow: short press = `CONFIRM`, long press = `CANCEL`
  - In normal mode short press: toggle freeze (`LIVE` <-> `FROZEN`)
  - In normal mode long press (~1.2s): cycle axis (`BOTH -> ROLL -> PITCH`)
  - In normal mode very long press (~2.2s): toggle orientation (`SCREEN UP` <-> `SCREEN VERTICAL`)
  - In normal mode ultra long press (~3.2s): start OFFSET CAL workflow
  - While holding in normal mode, an on-screen hint shows the release action and countdown to the next action threshold.
- Touch readout area:
  - Tap the roll/pitch value area to toggle freeze (`LIVE` <-> `FROZEN`)
- UI behavior:
  - `ZERO` is guided (`CONFIRM`/`CANCEL`, stillness timer + averaging progress bar).
  - `ZERO` long press starts OFFSET CAL workflow.
  - Button touch targets are extended to improve tap reliability.
  - `MODE` is immediate (loads mode-specific calibration/zero settings from EEPROM).
  - OFFSET CAL is guided (`CONFIRM`/`CANCEL`, stillness timer + progress bar).

## Remote Control (Phase A)

Phone remote control is now available through a device-hosted web UI over Wi-Fi AP mode.

How to connect:
1. Power the device.
2. Connect your phone to the device AP:
   - SSID: `IncidencePerfectNG-XXXX` (last 4 hex from chip id)
   - Password: `incidence-ng`
3. Open browser and go to:
   - `http://192.168.4.1`

Available in Phase A:
- Live readout (`ROLL`, `PITCH`, status line mirror)
- Remote actions:
  - `ZERO`
  - `AXIS`
  - `FREEZE`
  - `ROTATE`
  - `OFFSET CAL` workflow (`OFFSET CAL` -> `CONFIRM`/`CANCEL`)
  - `MODE` immediate toggle or direct set
  - `ALIGN` start + `CAPTURE` + `CANCEL`
- Context-aware controls:
  - `CONFIRM`/`CANCEL` are shown only during ZERO/OFFSET CAL workflows
  - `CAPTURE`/`CANCEL` are shown only during ALIGN workflow
- Progress bar:
  - ZERO hold/sampling progress
  - OFFSET CAL hold/sampling progress
  - ALIGN capture progress
- Roll conditioning hint:
  - web UI now shows a warning when roll reliability is reduced near high pitch angles
- Diagnostics panel:
  - expandable diagnostics card with raw/remapped/corrected IMU values
  - live offset/zero/alignment references and workflow flags
- Workflow sync is shared across touch, serial, ACTION button, and web.

API endpoints:
- `GET /api/state`
- `POST /api/cmd` with JSON body:
  - `{"cmd":"zero"|"axis"|"freeze"|"rotate"}`
  - `{"cmd":"offset_cal"|"confirm"|"cancel"}` (`zero`/`offset_cal` open guided workflows; `confirm`/`cancel` act on active workflow)
  - `{"cmd":"mode_toggle"|"mode_up"|"mode_vertical"}`
  - `{"cmd":"align_start"|"capture"|"cancel"}`
- `GET /health`

State payload highlights (`GET /api/state`):
- Main: roll/pitch, orientation, axis, rotation, live/frozen
- Workflow: zero/mode/offset-cal/align active + progress
- Diagnostics: sensor raw/remapped/corrected values, conditioning %, bias/zero/align refs

### `c` vs `C`

- Use `c` for context action:
  - in ZERO it confirms,
  - in normal mode it starts+confirms guided OFFSET CAL,
  - in OFFSET CAL it confirms,
  - in ALIGN it captures.
- Use `C` when you need full mechanical alignment (mounting/enclosure bias correction).
- In short: `c` handles guided runtime offset calibration actions; `C` is full alignment procedure.

## Versioning Policy

Firmware versions use the format `YYYY.M.X`.

Current firmware version is generated automatically at build time.

- `YYYY`: calendar year (for example `2026`)
- `M`: month number (`1`-`12`)
- `X`: incremental release/build number within the month (`1`, `2`, `3`, ...)

Build behavior:
- Default: `FW_VERSION` is auto-generated in PlatformIO as `YYYY.M.X`.
- `X` is computed from current-month commit count on `HEAD` (minimum `1`).
- Override for release candidates/special builds by setting env var before build:
  - PowerShell: `$env:FW_VERSION_OVERRIDE=\"2026.2.99-rc1\"`
  - then run `pio run` / `pio run -t upload`

Examples:
- `2026.2.1` = first firmware release in February 2026
- `2026.2.4` = fourth firmware release in February 2026
- `2026.3.1` = first firmware release in March 2026

## Notes

- Project config is in `platformio.ini`.
- Source files are in `src/` (standard PlatformIO layout).
- LVGL config lives at `config/lvgl/lv_conf.h` and is wired via build flags in `platformio.ini`.
- Hardware setup notes and reference image are under `docs/hardware/`.
- Splash screen design assets are under `docs/assets/splash-screens/`.
- Splash generation pipeline:
  - script: `scripts/generate_splash.ps1`
  - default flow: `crop-scale` (crop to aspect ratio + scale to `536x240`)
  - outputs:
    - device PNG in `docs/assets/splash-screens/device/`
    - firmware header `src/splash_image_536x240_rgb565.h`
- Architecture notes are under `docs/architecture/`.
- Validation checklists are under `docs/testing/`.
- End-user manual is under `docs/manual/`.
  - Build outputs: `docs/manual/dist/` (ignored)
  - Curated release PDFs: `docs/manual/releases/` (tracked)
- Current dependency pins in `platformio.ini`:
  - `lvgl/lvgl @ ^8.4.0`
  - `moononournation/GFX Library for Arduino @ 1.3.8`
  - `file://C:/dev/Arduino/Libraries/QMI8658`

## License

- Code license: MIT (`LICENSE`)
- Legal notes and scope details: `docs/legal/README.md`
- Third-party manuals/libraries keep their own original licenses/terms

## Repository layout

- `src/`: firmware source code
- `config/lvgl/`: LVGL configuration (`lv_conf.h`)
- `docs/hardware/`: board setup references
- `docs/assets/splash-screens/`: splash screen image assets
- `docs/architecture/`: design notes and diagrams
- `docs/testing/`: hardware validation workflow and test logs

## Backlog

1. Beta hardening
- Status: Partial
- Remaining:
  - Re-run full hardware validation on all beta units.
  - Confirm EEPROM persistence after repeated power cycles for orientation, rotation, alignment, and freeze behavior.

2. UX and control polish
- Status: Partial
- Done:
  - ACTION-button hold hint readability improved from field feedback.
  - On-screen ACTION-button long-press progress indicator added for short/long/very-long/ultra-long thresholds.
- Remaining:
  - Tune ACTION hold thresholds from real usage (`~1.2s / ~2.2s / ~3.2s`).

3. Documentation cleanup
- Status: Done
- Completed:
  - `docs/testing/hardware-validation-checklist.md` updated for guided ZERO/OFFSET CAL, immediate MODE, and full ALIGN flow.
  - `docs/testing/validation-session-template.md` updated with expanded current test matrix.

4. Connectivity exploration
- Status: Partial
- Done:
  - Phase A foundation implemented:
    - AP mode phone access
    - web UI + API telemetry/state
    - remote commands (`zero`, `axis`, `freeze`, `rotate`, `offset_cal`, `mode`, `align`)
- Remaining:
  - STA mode + hostname (`.local`) onboarding
  - auth hardening and network test matrix
- Plan document:
  - `docs/architecture/remote-control-plan.md`

5. Release/versioning
- Status: Done
- Completed:
  - Firmware version format `YYYY.M.X` adopted.
  - Build-time auto version generation implemented.
  - Optional override for release candidates via `FW_VERSION_OVERRIDE`.

6. Beta release process
- Status: Done
- Completed:
  - Added `docs/release/beta-checklist.md`.
  - Added `docs/release/tester-handoff-note.md`.
  - Added `docs/release/beta-version-and-tag-commands.md`.

7. 2010-to-NG improvement track (cleanup and observability)
- Status: In progress
- Priority ranking (best bang-for-buck first):
  - `P1` Command model cleanup:
    - keep `c` as primary context action
    - add explicit confirm/capture/cancel aliases so workflows are less ambiguous
  - `P2` Diagnostics visibility:
    - add runtime status dump (offsets/zero/alignment/workflow state)
    - add one-shot raw sample command in addition to 5 Hz stream toggle
  - `P3` Large-angle conditioning clarity:
    - expose roll-conditioning state to remote UI
    - warn users when roll reliability is reduced near high pitch angles
  - `P4` Guided diagnostics page:
    - add remote diagnostic panel for raw/corrected vectors and workflow state
  - `P5` Command grammar modernization:
    - optional future line-based parser for multi-character serial commands
  - `P6` Field tuning controls:
    - expose filter/sampling constants for validation builds
  - `P7` Optional theta channel:
    - evaluate whether legacy `theta` adds practical value for target use-cases
- Started now:
  - `P1` explicit serial aliases added (`y`, `p`, `n`) with legacy `c` preserved.
  - `P2` status/raw diagnostics added (`s`, `D`, help `h/?`).
  - `P3` roll-conditioning telemetry + web warning added.
