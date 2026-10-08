# ButterBot voice gateway - native install on Windows (no Docker). No admin rights needed.
# Re-running it upgrades the code and keeps config.yaml, secret.env and data\.
#
#   powershell -ExecutionPolicy Bypass -File .\install.ps1
#   powershell -ExecutionPolicy Bypass -File .\install.ps1 -App D:\bbgw -NoStartupTask
#   -Unattended: no prompts (default config if there is none yet). Remove: .\uninstall.ps1 [-Purge]
param(
    [string]$App = "$env:LOCALAPPDATA\bb-voice-gateway",
    [switch]$NoStartupTask,  # install only; start it yourself with run.ps1
    [switch]$Unattended
)
$ErrorActionPreference = "Stop"
$Src = Split-Path -Parent $MyInvocation.MyCommand.Path
$TaskName = "ButterBot Voice Gateway"

# Upgrading: a running gateway locks its Python files, so stop it first.
if (Get-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue) {
    Stop-ScheduledTask -TaskName $TaskName -ErrorAction SilentlyContinue
}
Get-CimInstance Win32_Process -Filter "Name='python.exe'" -ErrorAction SilentlyContinue |
    Where-Object { $_.ExecutablePath -and $_.ExecutablePath.StartsWith("$App\", [StringComparison]::OrdinalIgnoreCase) } |
    ForEach-Object { Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue }

# Python 3.10+ (python.org installer or `winget install Python.Python.3.12`)
$py = $null
foreach ($cand in @(@("py", "-3.12"), @("py", "-3.11"), @("py", "-3.10"), @("py", "-3"), @("python"))) {
    try {
        $v = & $cand[0] $cand[1..($cand.Length - 1)] -c "import sys; print(sys.version_info >= (3, 10))" 2>$null
        if ($v -eq "True") { $py = $cand; break }
    } catch { }
}
if (-not $py) { throw "Python 3.10+ not found. Install it with: winget install Python.Python.3.12" }

$version = if (Test-Path "$Src\VERSION") { (Get-Content "$Src\VERSION" -Raw).Trim() } else { "dev" }
Write-Host "== installing bb-voice-gateway $version to $App"
New-Item -ItemType Directory -Force "$App\data" | Out-Null
Copy-Item "$Src\*.py", "$Src\Scenarios.h", "$Src\config.example.yaml", "$Src\requirements.txt", "$Src\VERSION", "$Src\uninstall.ps1" $App -Force
foreach ($f in "config.yaml", "secret.env") {   # bring over a config made with setup.py
    if ((Test-Path "$Src\$f") -and -not (Test-Path "$App\$f")) { Copy-Item "$Src\$f" $App }
}

& $py[0] $py[1..($py.Length - 1)] -m venv "$App\.venv"
$vpy = "$App\.venv\Scripts\python.exe"
& $vpy -m pip install -q --upgrade pip
& $vpy -m pip install -q -r "$App\requirements.txt"

$dllDirs = @()
if (Get-Command nvidia-smi -ErrorAction SilentlyContinue) {
    $gpu = (& nvidia-smi --query-gpu=name --format=csv,noheader | Select-Object -First 1)
    Write-Host "== NVIDIA GPU: $gpu; adding CUDA 12 / cuDNN 9 libraries"
    & $vpy -m pip install -q nvidia-cublas-cu12 "nvidia-cudnn-cu12==9.*"
    $site = & $vpy -c "import site; print(site.getsitepackages()[-1])"
    $dllDirs = @("$site\nvidia\cublas\bin", "$site\nvidia\cudnn\bin") | Where-Object { Test-Path $_ }
}

if (-not (Test-Path "$App\config.yaml")) {
    $env:BB_INSTALLER = "1"
    if ($Unattended -or [Console]::IsInputRedirected) {
        Write-Host "== no config.yaml yet - writing a default one (edit it, or run setup.py, then restart)"
        & $vpy "$App\setup.py" --dir $App --yes
    } else {
        Write-Host "== no config.yaml yet - starting the setup wizard"
        & $vpy "$App\setup.py" --dir $App
    }
    if ($LASTEXITCODE -ne 0) { throw "setup.py failed" }
}
if (-not (Test-Path "$App\secret.env")) { New-Item -ItemType File "$App\secret.env" | Out-Null }

# Launcher: loads secret.env, puts the CUDA DLLs on PATH, logs to data\gateway.log.
$dllLine = if ($dllDirs) { "`$env:PATH = `"$($dllDirs -join ';');`$env:PATH`"" } else { "" }
@"
Set-Location "$App"
foreach (`$f in "secret.env", "admin.env") {
    if (Test-Path `$f) {
        Get-Content `$f | Where-Object { `$_ -match '^\s*[^#][^=]*=' } | ForEach-Object {
            `$k, `$v = `$_ -split '=', 2; [Environment]::SetEnvironmentVariable(`$k.Trim(), `$v, 'Process')
        }
    }
}
`$env:PYTHONUNBUFFERED = "1"; `$env:HF_HOME = "$App\data\hf"; `$env:BB_DATA_DIR = "$App\data"
$dllLine
# Start a fresh log when it's over 5 MB, or in the UTF-16 format older installers wrote.
`$log = "$App\data\gateway.log"
if (Test-Path `$log) {
    `$bom = (Get-Content `$log -Encoding Byte -TotalCount 2) -join ','
    if ((Get-Item `$log).Length -gt 5MB -or `$bom -eq '255,254') { Move-Item -Force `$log "`$log.1" }
}
# cmd does the redirection: PowerShell would wrap Python's (normal) stderr log lines as error records.
cmd /c "`"$vpy`" gateway.py --config `"$App\config.yaml`" >> `"$App\data\gateway.log`" 2>&1"
"@ | Set-Content -Encoding UTF8 "$App\run.ps1"

if (-not $NoStartupTask) {
    $action = New-ScheduledTaskAction -Execute "powershell.exe" `
        -Argument "-NoProfile -WindowStyle Hidden -ExecutionPolicy Bypass -File `"$App\run.ps1`""
    $trigger = New-ScheduledTaskTrigger -AtLogOn -User $env:USERNAME
    $settings = New-ScheduledTaskSettingsSet -ExecutionTimeLimit ([TimeSpan]::Zero) `
        -RestartCount 3 -RestartInterval (New-TimeSpan -Minutes 1) -AllowStartIfOnBatteries
    Register-ScheduledTask -TaskName $TaskName -Action $action -Trigger $trigger `
        -Settings $settings -Force | Out-Null
    Start-ScheduledTask -TaskName $TaskName
    Write-Host "== startup task '$TaskName' registered and started (runs at logon)"
}

$ip = (Get-NetIPConfiguration | Where-Object { $_.IPv4DefaultGateway } | Select-Object -First 1).IPv4Address.IPAddress
Write-Host ""
Write-Host "Gateway: ws://${ip}:8765/bb"
Write-Host "Logs:    Get-Content `"$App\data\gateway.log`" -Wait   (first start downloads the Whisper model)"
Write-Host "Allow ButterBot through the firewall (admin PowerShell, once):"
Write-Host "  New-NetFirewallRule -DisplayName `"ButterBot gateway`" -Direction Inbound -Protocol TCP -LocalPort 8765 -Action Allow -Profile Private"
