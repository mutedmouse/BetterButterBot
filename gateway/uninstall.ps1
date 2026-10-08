# Remove a native bb-voice-gateway install on Windows (startup task + program files).
# Keeps config.yaml, secret.env, admin.env and data\ (keys, calendar login, transcripts, models)
# unless you pass -Purge.
#   powershell -ExecutionPolicy Bypass -File .\uninstall.ps1 [-Purge] [-App D:\bbgw]
param(
    [string]$App = "$env:LOCALAPPDATA\bb-voice-gateway",
    [switch]$Purge
)
$ErrorActionPreference = "Stop"
$TaskName = "ButterBot Voice Gateway"

if (Get-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue) {
    Stop-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue
    Unregister-ScheduledTask -TaskName $TaskName -Confirm:$false
    Write-Host "== startup task removed"
}
Get-CimInstance Win32_Process -Filter "Name='python.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.ExecutablePath -and $_.ExecutablePath.StartsWith("$App\", [StringComparison]::OrdinalIgnoreCase) } |
    ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }

if (-not (Test-Path $App)) { Write-Host "Nothing installed at $App."; return }
# Refuse to delete anything that isn't a gateway install (guards against a wrong -App): either the
# full install, or what an earlier uninstall left behind (config, keys, data only).
$kept = @("config.yaml", "secret.env", "admin.env", "data")
$names = @(Get-ChildItem $App -Force | ForEach-Object Name)
$fullInstall = (Test-Path "$App\gateway.py") -and (Test-Path "$App\run.ps1")
$leftovers = ($names.Count -gt 0) -and -not ($names | Where-Object { $_ -notin $kept }) -and
             ((Test-Path "$App\config.yaml") -or (Test-Path "$App\data"))
if (-not ($fullInstall -or $leftovers)) {
    throw "$App doesn't look like a bb-voice-gateway install - not touching it."
}
Start-Sleep -Milliseconds 500   # let Windows release the files of the stopped process

if ($Purge) {
    Remove-Item $App -Recurse -Force
    Write-Host "== removed $App, including config, keys, calendar login and data"
} else {
    Get-ChildItem $App -Force | Where-Object { $_.Name -notin @("config.yaml", "secret.env", "admin.env", "data") } |
        Remove-Item -Recurse -Force
    Write-Host "== removed the program files; kept config.yaml, secret.env and data\ in $App"
    Write-Host "   (re-running install.ps1 picks them up again; uninstall.ps1 -Purge deletes them)"
}
