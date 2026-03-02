# Splash Screens

Canonical splash assets for firmware/manual generation.

Tracked files:
- `splash_screen_OLED_optimized_4.png` (canonical source image)
- `device/splash_screen_OLED_optimized_4_536x240.png` (device-sized generated output)

Notes:
- Additional exploratory variants should stay outside Git (or in local ignored paths).
- Firmware integration renders version text (`YYYY.M.X`) on top during boot.

## Generation Pipeline

Use the generator script to transform a source image into:
- device-ready PNG (`536x240`)
- firmware header (`src/splash_image_536x240_rgb565.h`)

Command (recommended default: crop + scale):

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\generate_splash.ps1 `
  -InputPath .\docs\assets\splash-screens\splash_screen_OLED_optimized_4.png `
  -Mode crop-scale
```

Then build/upload:

```powershell
pio run -e esp32s3 -t upload
```

Modes:
- `crop-scale` (default): crop source to target aspect ratio, then scale to `536x240`.
- `fill-width-crop`: scale to width 536, then crop height to 240.
- `fit-letterbox`: preserve full image, add black bars if needed.
- `center-crop`: direct 1:1 crop to `536x240` from source.

Optional focus controls:
- `-FocusX` and `-FocusY` in range `[0..1]` to bias crop/placement.
- `0.5` means centered (default).
