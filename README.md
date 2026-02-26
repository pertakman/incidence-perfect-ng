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
   - `ROTATE` flips 180 and persists after reboot

## Serial Commands

- `z`: set zero reference from current position
- `c`: quick sensor recalibration (`calibrateOffsets()` + `initializeAngles()`) when not aligning; during alignment it captures the current step
- `C`: start guided 6-step mechanical alignment workflow (shared by touch UI and serial)
- `u`: start guided mode change targeting `SCREEN UP` (confirm with `c`, cancel with `x`)
- `v`: start guided mode change targeting `SCREEN VERTICAL` (confirm with `c`, cancel with `x`)
- `m`: start guided mode change targeting the opposite orientation (confirm with `c`, cancel with `x`)
- `x`: cancel pending serial-guided mode change

Touch UI and serial now share the same MODE workflow state, so actions in one interface are reflected in the other.
- `a`: cycle axis display/output (`BOTH -> ROLL -> PITCH`)
- `r`: toggle 180-degree screen rotation

## Hardware Controls

- `ACTION button` (`GPIO0`, active-low, labeled `BOOT/GPO` on board):
  - In ALIGN workflow: press/release = `CAPTURE`
  - In MODE workflow: short press = `CONFIRM`, long press = `CANCEL`
  - In normal mode short press: toggle freeze (`LIVE` <-> `FROZEN`)
  - In normal mode long press (~1.2s): cycle axis (`BOTH -> ROLL -> PITCH`)
  - In normal mode very long press (~2.2s): toggle orientation (`SCREEN UP` <-> `SCREEN VERTICAL`)
  - While holding in normal mode, an on-screen hint shows the release action and countdown to the next action threshold.
- Touch readout area:
  - Tap the roll/pitch value area to toggle freeze (`LIVE` <-> `FROZEN`)
- UI behavior:
  - `ZERO` shows a short on-screen confirmation (`ZERO APPLIED - hold still briefly`).
  - Button touch targets are extended to improve tap reliability.
  - `MODE` is guided:
    - first press opens mode guidance with target orientation and changes `MODE` to `CONFIRM`,
    - first `CONFIRM` starts stillness countdown,
    - mode change auto-applies after countdown completes,
    - `ZERO` or ACTION-button long-press cancels a pending mode change.

### `c` vs `C`

- Use `c` for quick recalibration when values drift or after a temperature/stability change.
- Use `C` when you need full mechanical alignment (mounting/enclosure bias correction).
- In short: `c` is fast sensor re-zero; `C` is full alignment procedure.

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
  - On-screen ACTION-button long-press progress indicator added for short/long/very-long thresholds.
- Remaining:
  - Tune `MODE` guided workflow timing (`MODE_STILL_MS`, motion threshold) from real usage.

3. Documentation cleanup
- Status: Done
- Completed:
  - `docs/testing/hardware-validation-checklist.md` updated for guided MODE and full ALIGN flow.
  - `docs/testing/validation-session-template.md` updated with expanded current test matrix.

4. Connectivity exploration
- Status: Open
- Investigate smartphone connectivity options:
  - native app approach
  - lightweight web UI approach (served page or companion bridge)
- Define minimum viable remote UI scope (read-only telemetry first, then optional control actions).

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
