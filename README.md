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

## Notes

- Project config is in `platformio.ini`.
- Source files are in `src/` (standard PlatformIO layout).
- LVGL config lives at `config/lvgl/lv_conf.h` and is wired via build flags in `platformio.ini`.
- Hardware setup notes and reference image are under `docs/hardware/`.
- Architecture notes are under `docs/architecture/`.
- Current dependency pins in `platformio.ini`:
  - `lvgl/lvgl @ ^8.4.0`
  - `moononournation/GFX Library for Arduino @ 1.3.8`
  - `file://C:/dev/Arduino/Libraries/QMI8658`

## Repository layout

- `src/`: firmware source code
- `config/lvgl/`: LVGL configuration (`lv_conf.h`)
- `docs/hardware/`: board setup references
- `docs/architecture/`: design notes and diagrams
