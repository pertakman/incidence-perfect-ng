# Splash Screens

Candidate splash screen PNG assets for boot/version overlay work.

Files:
- `splash_screen_1.png`
- `splash_screen_2.png`
- `splash_screen_monochrome.png`
- `splash_screen_OLED_optimized.png`
- `splash_screen_pixel_grid.png`
- `splash_screen_ultra_minimal.png`
- `splash_screen_variants.png`

Notes:
- Keep these as source/design assets.
- Firmware integration can later choose one base image and render version text (`YYYY.M.X`) on top.

## Generation Pipeline

Use the generator script to transform a source image into:
- device-ready PNG (`536x240`)
- firmware header (`src/splash_image_536x240_rgb565.h`)

Command (recommended default: crop + scale):

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\generate_splash.ps1 `
  -InputPath .\docs\assets\splash-screens\splash_screen_OLED_optimized_2.png `
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
