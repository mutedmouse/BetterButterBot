# Install ESP-IDF v5.5.3 for BetterButterBot (Windows).
#   powershell -ExecutionPolicy Bypass -File tools\setup_idf.ps1
#   powershell -ExecutionPolicy Bypass -File tools\setup_idf.ps1 -IdfDir C:\esp\esp-idf-v5.5.3
# Needs Git and Python 3.9+ (see docs/ESP-IDF-SETUP.md). Alternative: Espressif's graphical
# "ESP-IDF Tools Installer" - pick v5.5.3 there instead of running this script.
param([string]$IdfDir = "$env:USERPROFILE\esp\esp-idf-v5.5.3")
$ErrorActionPreference = "Stop"
$IdfVersion = "v5.5.3"

$missing = @()
if (-not (Get-Command git -ErrorAction SilentlyContinue)) { $missing += "Git (winget install --id Git.Git -e)" }
$py = Get-Command python -ErrorAction SilentlyContinue
if (-not $py -or (& python --version 2>&1) -notmatch "Python 3\.(9|1\d)") { $missing += "Python 3.9-3.13 (winget install --id Python.Python.3.12 -e)" }
if ($missing) {
    Write-Host "Missing:"; $missing | ForEach-Object { Write-Host "  $_" }
    Write-Host "Install them, open a NEW PowerShell window, and run this script again."
    exit 1
}
if ($IdfDir.Length -gt 60 -or $IdfDir -match " ") {
    Write-Warning "ESP-IDF builds are happiest in a short path without spaces (e.g. C:\esp\esp-idf-v5.5.3)."
}

if (Test-Path "$IdfDir\.git") {
    $have = (git -C $IdfDir describe --tags 2>$null)
    if ($have -ne $IdfVersion) { throw "$IdfDir holds ESP-IDF $have, not $IdfVersion. Use -IdfDir to pick another folder." }
    Write-Host "ESP-IDF $IdfVersion already cloned at $IdfDir"
} else {
    New-Item -ItemType Directory -Force (Split-Path $IdfDir) | Out-Null
    Write-Host "Downloading ESP-IDF $IdfVersion into $IdfDir (about 1 GB)..."
    git -c core.longpaths=true clone -b $IdfVersion --depth 1 --recursive --shallow-submodules https://github.com/espressif/esp-idf.git $IdfDir
    if ($LASTEXITCODE -ne 0) { throw "git clone failed" }
}

Write-Host "Installing the ESP32-S3 toolchain (about 1-2 GB, a few minutes)..."
& "$IdfDir\install.ps1" esp32s3
if ($LASTEXITCODE -ne 0) { throw "install.ps1 failed" }

Write-Host ""
Write-Host "Done. In every new PowerShell window, before building:"
Write-Host "    . $IdfDir\export.ps1"
Write-Host "Check:   idf.py --version     (should print ESP-IDF $IdfVersion)"
Write-Host "If PowerShell refuses to run export.ps1, run once:  Set-ExecutionPolicy -Scope CurrentUser RemoteSigned"
