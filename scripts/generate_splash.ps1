param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,

    [string]$OutputPng,
    [string]$OutputHeader = "src/splash_image_536x240_rgb565.h",
    [int]$Width = 536,
    [int]$Height = 240,

    [ValidateSet("crop-scale", "fill-width-crop", "fit-letterbox", "center-crop")]
    [string]$Mode = "crop-scale",

    [double]$FocusX = 0.5,
    [double]$FocusY = 0.5,
    [switch]$SkipHeader
)

$ErrorActionPreference = "Stop"

if ($FocusX -lt 0 -or $FocusX -gt 1 -or $FocusY -lt 0 -or $FocusY -gt 1) {
    throw "FocusX/FocusY must be in range [0, 1]."
}

Add-Type -AssemblyName System.Drawing

function Clamp-Int {
    param([int]$Value, [int]$Min, [int]$Max)
    if ($Value -lt $Min) { return $Min }
    if ($Value -gt $Max) { return $Max }
    return $Value
}

$inputAbs = (Resolve-Path $InputPath).Path

if (-not $OutputPng) {
    $baseName = [System.IO.Path]::GetFileNameWithoutExtension($inputAbs)
    $OutputPng = "docs/assets/splash-screens/device/${baseName}_${Width}x${Height}.png"
}

$outputPngAbs = Join-Path (Resolve-Path ".").Path $OutputPng
$outputPngDir = Split-Path $outputPngAbs -Parent
if (-not (Test-Path $outputPngDir)) {
    New-Item -ItemType Directory -Path $outputPngDir | Out-Null
}

$outputHeaderAbs = Join-Path (Resolve-Path ".").Path $OutputHeader
$outputHeaderDir = Split-Path $outputHeaderAbs -Parent
if (-not (Test-Path $outputHeaderDir)) {
    New-Item -ItemType Directory -Path $outputHeaderDir | Out-Null
}

$src = [System.Drawing.Bitmap]::new($inputAbs)
try {
    $dst = [System.Drawing.Bitmap]::new($Width, $Height, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
    try {
        $g = [System.Drawing.Graphics]::FromImage($dst)
        try {
            $g.Clear([System.Drawing.Color]::Black)
            $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
            $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
            $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality

            $srcW = [double]$src.Width
            $srcH = [double]$src.Height
            $targetAspect = [double]$Width / [double]$Height
            $srcAspect = $srcW / $srcH

            if ($Mode -eq "crop-scale") {
                if ($srcAspect -gt $targetAspect) {
                    $cropW = [int][Math]::Round($srcH * $targetAspect)
                    $cropH = [int]$srcH
                    $maxLeft = [int]$srcW - $cropW
                    $cropLeft = Clamp-Int -Value ([int][Math]::Round($maxLeft * $FocusX)) -Min 0 -Max $maxLeft
                    $cropTop = 0
                } else {
                    $cropW = [int]$srcW
                    $cropH = [int][Math]::Round($srcW / $targetAspect)
                    $maxTop = [int]$srcH - $cropH
                    $cropTop = Clamp-Int -Value ([int][Math]::Round($maxTop * $FocusY)) -Min 0 -Max $maxTop
                    $cropLeft = 0
                }

                $srcRect = [System.Drawing.Rectangle]::new($cropLeft, $cropTop, $cropW, $cropH)
                $dstRect = [System.Drawing.Rectangle]::new(0, 0, $Width, $Height)
                $g.DrawImage($src, $dstRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
            }
            elseif ($Mode -eq "fill-width-crop") {
                $scaledW = $Width
                $scaledH = [int][Math]::Round($srcH * ($Width / $srcW))
                if ($scaledH -lt $Height) {
                    throw "fill-width-crop failed: scaled height ($scaledH) < target height ($Height)."
                }

                $temp = [System.Drawing.Bitmap]::new($scaledW, $scaledH, [System.Drawing.Imaging.PixelFormat]::Format24bppRgb)
                try {
                    $tg = [System.Drawing.Graphics]::FromImage($temp)
                    try {
                        $tg.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                        $tg.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
                        $tg.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                        $tg.DrawImage($src, [System.Drawing.Rectangle]::new(0, 0, $scaledW, $scaledH))
                    }
                    finally {
                        $tg.Dispose()
                    }

                    $maxTop = $scaledH - $Height
                    $cropTop = Clamp-Int -Value ([int][Math]::Round($maxTop * $FocusY)) -Min 0 -Max $maxTop
                    $srcRect = [System.Drawing.Rectangle]::new(0, $cropTop, $Width, $Height)
                    $dstRect = [System.Drawing.Rectangle]::new(0, 0, $Width, $Height)
                    $g.DrawImage($temp, $dstRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
                }
                finally {
                    $temp.Dispose()
                }
            }
            elseif ($Mode -eq "fit-letterbox") {
                $scale = [Math]::Min($Width / $srcW, $Height / $srcH)
                $drawW = [int][Math]::Round($srcW * $scale)
                $drawH = [int][Math]::Round($srcH * $scale)
                $drawX = [int][Math]::Round(($Width - $drawW) * $FocusX)
                $drawY = [int][Math]::Round(($Height - $drawH) * $FocusY)
                $drawX = Clamp-Int -Value $drawX -Min 0 -Max ($Width - $drawW)
                $drawY = Clamp-Int -Value $drawY -Min 0 -Max ($Height - $drawH)

                $g.DrawImage($src, [System.Drawing.Rectangle]::new($drawX, $drawY, $drawW, $drawH))
            }
            else {
                if ($src.Width -lt $Width -or $src.Height -lt $Height) {
                    throw "center-crop mode requires source >= target dimensions."
                }
                $maxLeft = $src.Width - $Width
                $maxTop = $src.Height - $Height
                $cropLeft = Clamp-Int -Value ([int][Math]::Round($maxLeft * $FocusX)) -Min 0 -Max $maxLeft
                $cropTop = Clamp-Int -Value ([int][Math]::Round($maxTop * $FocusY)) -Min 0 -Max $maxTop
                $srcRect = [System.Drawing.Rectangle]::new($cropLeft, $cropTop, $Width, $Height)
                $dstRect = [System.Drawing.Rectangle]::new(0, 0, $Width, $Height)
                $g.DrawImage($src, $dstRect, $srcRect, [System.Drawing.GraphicsUnit]::Pixel)
            }
        }
        finally {
            $g.Dispose()
        }

        $dst.Save($outputPngAbs, [System.Drawing.Imaging.ImageFormat]::Png)

        if (-not $SkipHeader) {
            $sb = [System.Text.StringBuilder]::new()
            [void]$sb.AppendLine("#pragma once")
            [void]$sb.AppendLine("")
            [void]$sb.AppendLine("static const uint16_t SPLASH_IMAGE_536x240_RGB565[536 * 240] = {")

            $count = 0
            for ($y = 0; $y -lt $Height; $y++) {
                for ($x = 0; $x -lt $Width; $x++) {
                    $p = $dst.GetPixel($x, $y)
                    $rgb565 = (($p.R -band 0xF8) -shl 8) -bor (($p.G -band 0xFC) -shl 3) -bor (($p.B -band 0xF8) -shr 3)

                    if (($count % 12) -eq 0) {
                        [void]$sb.Append("    ")
                    }
                    [void]$sb.Append(("0x{0:X4}, " -f $rgb565))
                    $count++
                    if (($count % 12) -eq 0) {
                        [void]$sb.AppendLine()
                    }
                }
            }
            if (($count % 12) -ne 0) {
                [void]$sb.AppendLine()
            }
            [void]$sb.AppendLine("};")
            Set-Content -Path $outputHeaderAbs -Value $sb.ToString() -Encoding ascii
        }
    }
    finally {
        $dst.Dispose()
    }
}
finally {
    $src.Dispose()
}

Write-Output ("[splash] source: {0}" -f $inputAbs)
Write-Output ("[splash] mode: {0}" -f $Mode)
Write-Output ("[splash] output png: {0}" -f $OutputPng)
if (-not $SkipHeader) {
    Write-Output ("[splash] output header: {0}" -f $OutputHeader)
}
Write-Output ("[splash] done: {0}x{1}" -f $Width, $Height)
