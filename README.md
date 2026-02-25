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
- `c`: quick sensor recalibration (`calibrateOffsets()` + `initializeAngles()`)
- `C`: full mechanical alignment workflow (`runAlignmentCalibration()`)
- `u`: force `SCREEN UP` orientation mode
- `v`: force `SCREEN VERTICAL` orientation mode
- `m`: toggle orientation mode (`u/v`)
- `a`: cycle axis display/output (`BOTH -> ROLL -> PITCH`)
- `r`: toggle 180-degree screen rotation

### `c` vs `C`

- Use `c` for quick recalibration when values drift or after a temperature/stability change.
- Use `C` when you need full mechanical alignment (mounting/enclosure bias correction).
- In short: `c` is fast sensor re-zero; `C` is full alignment procedure.

## Notes

- Project config is in `platformio.ini`.
- Source files are in `src/` (standard PlatformIO layout).
- LVGL config lives at `config/lvgl/lv_conf.h` and is wired via build flags in `platformio.ini`.
- Hardware setup notes and reference image are under `docs/hardware/`.
- Architecture notes are under `docs/architecture/`.
- Validation checklists are under `docs/testing/`.
- Current dependency pins in `platformio.ini`:
  - `lvgl/lvgl @ ^8.4.0`
  - `moononournation/GFX Library for Arduino @ 1.3.8`
  - `file://C:/dev/Arduino/Libraries/QMI8658`

## Repository layout

- `src/`: firmware source code
- `config/lvgl/`: LVGL configuration (`lv_conf.h`)
- `docs/hardware/`: board setup references
- `docs/architecture/`: design notes and diagrams
- `docs/testing/`: hardware validation workflow and test logs
