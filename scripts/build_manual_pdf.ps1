param(
    [string]$ManualPath = "docs/manual/user-manual.md",
    [string]$OutDir = "docs/manual/dist",
    [string]$ToolsDir = "tools"
)

$ErrorActionPreference = "Stop"

function Get-FwVersion {
    if ($env:FW_VERSION_OVERRIDE -and $env:FW_VERSION_OVERRIDE.Trim().Length -gt 0) {
        return $env:FW_VERSION_OVERRIDE.Trim()
    }

    $now = Get-Date
    $year = $now.Year
    $month = $now.Month
    $since = Get-Date -Year $year -Month $month -Day 1 -Hour 0 -Minute 0 -Second 0

    # Count commits since month start (local time). Minimum 1.
    $count = 0
    try {
        $count = (git log --since $since.ToString("yyyy-MM-ddTHH:mm:ss") --pretty=oneline | Measure-Object -Line).Lines
    } catch {
        $count = 1
    }
    if (-not $count -or $count -lt 1) { $count = 1 }

    return "$year.$month.$count"
}

function Get-GitDescribe {
    try {
        $d = (git describe --tags --always 2>$null).Trim()
        if ($d) { return $d }
    } catch {}
    try {
        return (git rev-parse --short HEAD).Trim()
    } catch {
        return "unknown"
    }
}

function Get-BrowserExe {
    $candidates = @(
        "C:\Program Files\Google\Chrome\Application\chrome.exe",
        "C:\Program Files (x86)\Google\Chrome\Application\chrome.exe",
        "C:\Program Files\Microsoft\Edge\Application\msedge.exe",
        "C:\Program Files (x86)\Microsoft\Edge\Application\msedge.exe"
    )
    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return $candidate
        }
    }
    return $null
}

New-Item -ItemType Directory -Force -Path $OutDir | Out-Null
$outDirAbs = (Resolve-Path $OutDir).Path

$fw = Get-FwVersion
$date = (Get-Date).ToString("yyyy-MM-dd")
$rev = Get-GitDescribe

# Ensure local tools exist
powershell -ExecutionPolicy Bypass -File (Join-Path "scripts" "ensure_pandoc.ps1") -ToolsDir $ToolsDir | Out-Null
$pandoc = Join-Path $ToolsDir "pandoc\\pandoc.exe"

if (-not (Test-Path $pandoc)) { throw "pandoc not found at $pandoc" }
$browser = Get-BrowserExe
if (-not $browser) { throw "No Chrome/Edge browser found for headless PDF rendering." }

$src = (Get-Content $ManualPath -Raw)
$src = $src.Replace("{{FW_VERSION}}", $fw)
$src = $src.Replace("{{MANUAL_DATE}}", $date)
$src = $src.Replace("{{GIT_DESCRIBE}}", $rev)

# Make the cover image link robust by copying it into the output folder.
$manualDir = Split-Path (Resolve-Path $ManualPath).Path -Parent
$coverRel = "../assets/splash-screens/device/splash_screen_OLED_optimized_2_536x240.png"
$coverPath = $null
try {
    $coverPath = (Resolve-Path (Join-Path $manualDir $coverRel)).Path
} catch {
    $coverPath = $null
}
if ($coverPath -and (Test-Path $coverPath)) {
    $coverOut = Join-Path $OutDir "cover.png"
    Copy-Item -Force $coverPath $coverOut
    $src = $src.Replace("($coverRel)", "(cover.png)")
}

$outMd = Join-Path $OutDir ("Incidence-Perfect-NG-Manual-$fw.md")
$outHtml = Join-Path $OutDir ("Incidence-Perfect-NG-Manual-$fw.html")
$outPdf = Join-Path $OutDir ("Incidence-Perfect-NG-Manual-$fw.pdf")

Set-Content -Path $outMd -Value $src -Encoding UTF8

Write-Output "[manual] md: $outMd"
Write-Output "[manual] html: $outHtml"
Write-Output "[manual] pdf: $outPdf"

& $pandoc $outMd -o $outHtml --standalone --toc --metadata title="Incidence Perfect NG" --metadata date="$date" --resource-path="$outDirAbs"

$htmlAbs = (Resolve-Path $outHtml).Path
$htmlUri = "file:///" + ($htmlAbs.Replace('\', '/'))
$pdfAbs = (Resolve-Path (Split-Path $outPdf -Parent)).Path + "\" + (Split-Path $outPdf -Leaf)

& $browser "--headless=new" "--disable-gpu" "--allow-file-access-from-files" "--print-to-pdf=$pdfAbs" $htmlUri | Out-Null

Write-Output "[manual] done"
