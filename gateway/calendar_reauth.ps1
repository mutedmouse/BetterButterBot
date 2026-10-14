# Renew ButterBot's Google Calendar login (needed every 7 days while the OAuth app is in "Testing").
# Opens the browser sign-in, puts the new token where the gateway reads it, and removes the local copy.
#   powershell -ExecutionPolicy Bypass -File .\calendar_reauth.ps1 -GatewayHost user@192.168.1.50
#   powershell -ExecutionPolicy Bypass -File .\calendar_reauth.ps1 -GatewayHost local    # gateway on this PC
# Linux/macOS: ./calendar_reauth.sh
param(
    [switch]$ReadOnly,   # default: see and edit events (ButterBot can add events)
    [string]$Client = "$PSScriptRoot\secrets\google_client.json",
    # SSH host of the gateway (e.g. user@192.168.1.50 or an ~/.ssh/config alias), or "local" for a
    # gateway running on this machine (Docker or native; the token goes to .\data). Default: $env:BB_GATEWAY_SSH.
    [string]$GatewayHost = $env:BB_GATEWAY_SSH,
    [string]$RemoteDir = "/opt/bb-voice-gateway/data"   # data directory on the gateway host (native Linux install)
)
$ErrorActionPreference = "Stop"
if (-not $GatewayHost) { throw "Say where the gateway runs: -GatewayHost user@<gateway-ip> (or -GatewayHost local), or set BB_GATEWAY_SSH." }
if (-not (Test-Path $Client)) { throw "OAuth client JSON not found: $Client (download it from Google Cloud Console, see docs/INSTALL.md)" }
$token = "$PSScriptRoot\secrets\google_token.json"
# calendar_auth.py only needs the standard library: use the gateway's venv when it works, otherwise any
# Python on PATH (a venv's launcher fails if the Python it was made from has moved or is unreachable).
$python = "python"
$venvPy = "$PSScriptRoot\.venv\Scripts\python.exe"
if (Test-Path $venvPy) {
    # Windows PowerShell treats a native program's error output as a terminating error under "Stop":
    # probe with errors non-fatal, so a broken venv just falls back.
    $ErrorActionPreference = "Continue"
    try {
        & $venvPy -c "import sys" 2>$null | Out-Null
        if ($LASTEXITCODE -eq 0) { $python = $venvPy }
    } catch { }
    $ErrorActionPreference = "Stop"
}
if ($python -eq "python" -and -not (Get-Command python -ErrorAction SilentlyContinue)) {
    throw "No working Python found: install Python 3.10+ (python.org), or repair the gateway's .venv."
}

$authArgs = [System.Collections.Generic.List[string]]::new()
$authArgs.Add("-u")   # unbuffered: show the sign-in link right away
$authArgs.Add("$PSScriptRoot\calendar_auth.py")
if (-not $ReadOnly) { $authArgs.Add("--write") }
$authArgs.Add($Client)
$authArgs.Add($token)
& $python $authArgs.ToArray()
if ($LASTEXITCODE -ne 0) { throw "sign-in failed" }

if ($GatewayHost -eq "local") {
    New-Item -ItemType Directory -Force "$PSScriptRoot\data" | Out-Null
    Move-Item -Force $token "$PSScriptRoot\data\google_token.json"
    Write-Host "Calendar login renewed in $PSScriptRoot\data (no restart needed)."
    return
}
scp -q $token "${GatewayHost}:/tmp/google_token.json"
ssh $GatewayHost "install -m 600 /tmp/google_token.json $RemoteDir/google_token.json && rm -f /tmp/google_token.json"
if ($LASTEXITCODE -ne 0) { throw "copy to $GatewayHost failed" }
Remove-Item $token
Write-Host "Calendar login renewed on $GatewayHost (no restart needed)."
