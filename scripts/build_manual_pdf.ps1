param(
    [string]$ManualPath = "docs/manual/user-manual.md",
    [string]$OutDir = "docs/manual/dist",
    [string]$ToolsDir = "tools",
    [switch]$PublishRelease,
    [string]$ReleaseDir = "docs/manual/releases",
    [string]$CoverTemplatePath = "docs/manual/cover-template.html",
    [string]$CssPath = "docs/manual/manual.css"
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
if (-not (Test-Path $CoverTemplatePath)) { throw "cover template not found at $CoverTemplatePath" }
if (-not (Test-Path $CssPath)) { throw "manual CSS not found at $CssPath" }

$src = (Get-Content $ManualPath -Raw)
$src = $src.Replace("{{FW_VERSION}}", $fw)
$src = $src.Replace("{{MANUAL_DATE}}", $date)
$src = $src.Replace("{{GIT_DESCRIBE}}", $rev)

function Copy-ManualImageAssets {
    param(
        [string]$Markdown,
        [string]$ManualDir,
        [string]$OutDir
    )

    $imgRegex = '!\[[^\]]*\]\(([^)\s]+)(?:\s+"[^"]*")?\)'
    $htmlImgRegex = '<img[^>]*\ssrc=["'']([^"'']+)["''][^>]*>'
    $assetMap = @{}

    $allPaths = New-Object System.Collections.Generic.List[string]

    $mdMatches = [regex]::Matches($Markdown, $imgRegex)
    foreach ($m in $mdMatches) {
        $allPaths.Add($m.Groups[1].Value.Trim())
    }

    $htmlMatches = [regex]::Matches($Markdown, $htmlImgRegex)
    foreach ($m in $htmlMatches) {
        $allPaths.Add($m.Groups[1].Value.Trim())
    }

    foreach ($rawPath in $allPaths) {
        if ([string]::IsNullOrWhiteSpace($rawPath)) { continue }
        if ($rawPath.StartsWith("http://") -or $rawPath.StartsWith("https://") -or $rawPath.StartsWith("data:")) { continue }
        if ($rawPath.StartsWith("#")) { continue }

        $resolved = $null
        try {
            $resolved = (Resolve-Path (Join-Path $ManualDir $rawPath)).Path
        } catch {
            $resolved = $null
        }
        if (-not $resolved -or -not (Test-Path $resolved)) { continue }

        $destLeaf = [IO.Path]::GetFileName($resolved)

        Copy-Item -Force $resolved (Join-Path $OutDir $destLeaf)
        $assetMap[$rawPath] = $destLeaf
    }

    foreach ($k in $assetMap.Keys) {
        $Markdown = $Markdown.Replace("($k)", "($($assetMap[$k]))")
        $Markdown = $Markdown.Replace("src=`"$k`"", "src=`"$($assetMap[$k])`"")
        $Markdown = $Markdown.Replace("src='$k'", "src='$($assetMap[$k])'")
    }
    return $Markdown
}

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

$src = Copy-ManualImageAssets -Markdown $src -ManualDir $manualDir -OutDir $OutDir

$outMd = Join-Path $OutDir ("Incidence-Perfect-NG-Manual-$fw.md")
$outHtml = Join-Path $OutDir ("Incidence-Perfect-NG-Manual-$fw.html")
$outPdf = Join-Path $OutDir ("Incidence-Perfect-NG-Manual-$fw.pdf")
$coverOut = Join-Path $OutDir "cover.png"
$coverInclude = Join-Path $OutDir "cover.generated.html"

Set-Content -Path $outMd -Value $src -Encoding UTF8

$coverTemplate = Get-Content $CoverTemplatePath -Raw
$coverTemplate = $coverTemplate.Replace("{{COVER_IMAGE}}", "cover.png")
$coverTemplate = $coverTemplate.Replace("{{FW_VERSION}}", $fw)
$coverTemplate = $coverTemplate.Replace("{{MANUAL_DATE}}", $date)
$coverTemplate = $coverTemplate.Replace("{{GIT_DESCRIBE}}", $rev)
Set-Content -Path $coverInclude -Value $coverTemplate -Encoding UTF8

Write-Output "[manual] md: $outMd"
Write-Output "[manual] html: $outHtml"
Write-Output "[manual] pdf: $outPdf"

$cssAbs = (Resolve-Path $CssPath).Path
$coverIncludeAbs = (Resolve-Path $coverInclude).Path
& $pandoc $outMd -o $outHtml --standalone --toc --toc-depth=2 --metadata title="Incidence Perfect NG" --metadata date="$date" --resource-path="$outDirAbs" --css="$cssAbs" --include-before-body="$coverIncludeAbs"

$htmlAbs = (Resolve-Path $outHtml).Path
$htmlUri = "file:///" + ($htmlAbs.Replace('\', '/'))
$pdfAbs = (Resolve-Path (Split-Path $outPdf -Parent)).Path + "\" + (Split-Path $outPdf -Leaf)

& $browser "--headless=new" "--disable-gpu" "--allow-file-access-from-files" "--export-tagged-pdf" "--print-to-pdf=$pdfAbs" $htmlUri | Out-Null

Write-Output "[manual] done"

if ($PublishRelease) {
    New-Item -ItemType Directory -Force -Path $ReleaseDir | Out-Null
    $releasePdf = Join-Path $ReleaseDir (Split-Path $outPdf -Leaf)
    Copy-Item -Force $outPdf $releasePdf
    Write-Output "[manual] release pdf: $releasePdf"
}
