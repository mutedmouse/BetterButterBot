# Preparing the ESP-IDF build environment

The robot firmware is built with **ESP-IDF v5.5.3**, Espressif's toolchain for the ESP32. This must be **exactly 5.5.3**: the project depends on fixes that older versions don't have. You need ESP-IDF once per computer. Allow about 3 GB of disk and 15–30 minutes, mostly downloading.

| | Windows 10/11 | Linux | macOS |
|---|---|---|---|
| Status | ✅ tested (Windows 11) | ✅ tested (Ubuntu 24.04, fresh install) | ⚠ not tested yet: same steps as Linux, should work |
| Setup script | `tools\setup_idf.ps1` | `tools/setup_idf.sh` | `tools/setup_idf.sh` |
| Activate per terminal | `. <idf>\export.ps1` | `. <idf>/export.sh` | `. <idf>/export.sh` |
| USB driver | CH340 (often automatic) | built in | CH340 (needed on older macOS) |

The setup scripts are in `firmware/tools`. Run the commands below from the repository's `firmware` folder.

---

## Windows

### 1. Prerequisites

You need **Git** and **Python 3.9–3.13**. In PowerShell:

```
winget install --id Git.Git -e
winget install --id Python.Python.3.12 -e
```

Close PowerShell and open a new window, so it picks up both programs.

### 2. Install ESP-IDF

**Option A: script (recommended).** From the firmware folder:

```
powershell -ExecutionPolicy Bypass -File tools\setup_idf.ps1
```

- It installs into `%USERPROFILE%\esp\esp-idf-v5.5.3`.
- If your user folder has a space in it or is long, choose a short path instead: add `-IdfDir C:\esp\esp-idf-v5.5.3`.

**Option B: Espressif's installer.** Download the *ESP-IDF Tools Installer* from Espressif's "Get Started" page for ESP-IDF, choose **v5.5.3**, and use the *ESP-IDF 5.5 PowerShell* shortcut it creates. Skip step 3 below; that shortcut activates ESP-IDF for you.

### 3. Activate it (every new PowerShell window)

```
. $env:USERPROFILE\esp\esp-idf-v5.5.3\export.ps1
idf.py --version          # ESP-IDF v5.5.3
```

If PowerShell refuses with "running scripts is disabled", run `Set-ExecutionPolicy -Scope CurrentUser RemoteSigned` once.

### 4. USB

Plug in the robot and turn it on. Device Manager → *Ports (COM & LPT)* should show **USB-SERIAL CH340 (COMx)**. If instead you see an unknown device, install the CH340 driver from the chip maker, WCH (search "CH341SER").

**Windows tips:**
- Keep the firmware folder short and free of spaces (`C:\bbb\BetterButterBot-firmware`). Very long paths break the build.
- Antivirus scanning makes the first build much slower. Adding the ESP-IDF and project folders as exclusions helps.
- **WSL:** you can build in WSL with the Linux steps, but WSL can't see the robot's USB port without extra tools (`usbipd`). Flash from Windows instead.

---

## Linux

### 1. Prerequisites

**Debian / Ubuntu / Mint:**
```
sudo apt install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0
```
**Fedora:**
```
sudo dnf install git wget flex bison gperf python3 cmake ninja-build ccache dfu-util libusbx
```
**Arch:**
```
sudo pacman -S --needed git wget flex bison gperf python cmake ninja ccache dfu-util libusb
```

### 2. Install ESP-IDF

From the firmware folder:

```
bash tools/setup_idf.sh
```

It installs into `~/esp/esp-idf-v5.5.3`. To use a different folder: `IDF_DIR=/path/to/esp-idf bash tools/setup_idf.sh`.

### 3. Activate it (every new terminal)

```
. ~/esp/esp-idf-v5.5.3/export.sh
idf.py --version          # ESP-IDF v5.5.3
```

To save typing, run `echo 'alias get_idf=". ~/esp/esp-idf-v5.5.3/export.sh"' >> ~/.bashrc` once; after that, `get_idf` activates it.

### 4. USB

The robot appears as `/dev/ttyUSB0`. Check with `ls /dev/ttyUSB*` while it's on. To use the port without `sudo`, run this once, then log out and back in:

```
sudo usermod -aG dialout $USER
```

On Arch the group is `uucp`. If `brltty` grabs the CH340 (the port appears briefly, then vanishes), remove it: `sudo apt remove brltty`.

---

## macOS (best effort)

Not yet tested on a Mac. These are ESP-IDF's standard macOS steps.

### 1. Prerequisites

Install [Homebrew](https://brew.sh), then:

```
brew install git python cmake ninja dfu-util
```

Apple Silicon Macs also need Rosetta for some ESP-IDF tools: `softwareupdate --install-rosetta --agree-to-license`.

### 2. Install ESP-IDF

```
bash tools/setup_idf.sh
```

### 3. Activate it (every new terminal)

```
. ~/esp/esp-idf-v5.5.3/export.sh
idf.py --version
```

### 4. USB

The port is `/dev/cu.wchusbserial…` or `/dev/cu.usbserial…`; check with `ls /dev/cu.*` while the robot is on. Recent macOS versions include the CH340 driver. On older versions, install WCH's CH34x driver and allow it under *System Settings → Privacy & Security*.

---

## Checking the setup

With ESP-IDF activated, in the firmware folder:

```
idf.py --version                                   # ESP-IDF v5.5.3
python tools/configure_robot.py --show             # works = Python is fine
idf.py build                                       # first build: 5-15 min, ends with "Project build complete"
```

Then continue with [INSTALL.md](INSTALL.md#b-set-your-wi-fi-network-and-gateway-address) step 3b.

## Problems

| Symptom | Fix |
|---|---|
| `idf.py: command not found` / `not recognized` | ESP-IDF isn't activated in this terminal. Run the `export` command from step 3. |
| `idf.py --version` shows another version | You have a different ESP-IDF activated (Arduino IDE / PlatformIO / an older install). Open a fresh terminal and activate the 5.5.3 one. |
| `install.sh`/`install.ps1` fails downloading tools | A network or proxy problem. Run it again; it continues where it stopped. |
| Python errors during install | Use Python 3.9–3.13. If the tool environment broke, delete `~/.espressif/python_env` (or `%USERPROFILE%\.espressif\python_env`) and run the install again. |
| Build fails with very long file names (Windows) | Move the firmware folder to a short path, e.g. `C:\bbb`. |
| "Permission denied" running a script on Linux/macOS (e.g. after Download ZIP) | `chmod +x tools/*.sh`, or run scripts with `bash tools/…`. |
