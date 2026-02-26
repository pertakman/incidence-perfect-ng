param(
    [string]$ToolsDir = "tools"
)

$ErrorActionPreference = "Stop"

$pandocDir = Join-Path $ToolsDir "pandoc"
$pandocExe = Join-Path $pandocDir "pandoc.exe"

if (Test-Path $pandocExe) {
    Write-Output "[pandoc] ok: $pandocExe"
    exit 0
}

New-Item -ItemType Directory -Force -Path $pandocDir | Out-Null

# Pandoc is a single-file CLI; we keep it local to the repo so builds are repeatable.
# Source: GitHub releases (direct asset link).
$version = "3.9"
$zipName = "pandoc-$version-windows-x86_64.zip"
$url = "https://github.com/jgm/pandoc/releases/download/$version/$zipName"
$zipPath = Join-Path $pandocDir $zipName

if (Test-Path $zipPath) {
    Remove-Item $zipPath -Force
}

Write-Output "[pandoc] downloading $url"
Invoke-WebRequest -Uri $url -OutFile $zipPath

Write-Output "[pandoc] extracting $zipName"
Expand-Archive -Path $zipPath -DestinationPath $pandocDir -Force

Remove-Item $zipPath -Force

# Zip contains a top-level folder; locate pandoc.exe and move it into tools/pandoc/.
$found = Get-ChildItem -Path $pandocDir -Recurse -Filter pandoc.exe | Select-Object -First 1
if (-not $found) {
    throw "[pandoc] extraction succeeded but pandoc.exe not found"
}

Copy-Item -Force $found.FullName $pandocExe

Write-Output "[pandoc] ready: $pandocExe"
