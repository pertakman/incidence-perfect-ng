# Board Settings (Arduino IDE)

This folder keeps reference hardware/tool settings used during bring-up.

- `board-settings-arduino-ide.jpg`: screenshot of Arduino IDE board configuration known to work with this hardware.

## Extracted Settings From Screenshot

- `USB CDC On Boot`: `Enabled`
- `CPU Frequency`: `240MHz (WiFi)`
- `Core Debug Level`: `None`
- `USB DFU On Boot`: `Disabled`
- `Erase All Flash Before Sketch Upload`: `Disabled`
- `Events Run On`: `Core 1`
- `Flash Mode`: `QIO 80MHz`
- `Flash Size`: `16MB (128Mb)`
- `JTAG Adapter`: `Disabled`
- `Arduino Runs On`: `Core 1`
- `USB Firmware MSC On Boot`: `Disabled`
- `Partition Scheme`: `Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)`
- `PSRAM`: `OPI PSRAM`
- `Upload Mode`: `UART0 / Hardware CDC`
- `Upload Speed`: `921600`
- `USB Mode`: `Hardware CDC and JTAG`

Items shown but without a value in the screenshot:
- `Programmer`
- `Burn Bootloader`

Notes:
- The active development workflow for this repository is VS Code + PlatformIO.
- Keep this screenshot for cross-checking if Arduino IDE is used for troubleshooting or comparison.
