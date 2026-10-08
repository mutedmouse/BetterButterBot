#!/usr/bin/env bash
# Install ESP-IDF v5.5.3 for BetterButterBot (Linux and macOS).
#   ./tools/setup_idf.sh              # installs into ~/esp/esp-idf-v5.5.3
#   IDF_DIR=/opt/esp-idf ./tools/setup_idf.sh
# Needs: git, python3 (3.9+), cmake, ninja - see docs/ESP-IDF-SETUP.md for the one-line install per OS.
set -euo pipefail
IDF_VERSION=v5.5.3
IDF_DIR="${IDF_DIR:-$HOME/esp/esp-idf-v5.5.3}"

missing=()
for tool in git python3 cmake ninja; do command -v "$tool" >/dev/null || missing+=("$tool"); done
if [ ${#missing[@]} -gt 0 ]; then
  echo "Missing: ${missing[*]}"
  if [ "$(uname)" = Darwin ]; then
    echo "Install with:  brew install git python cmake ninja dfu-util"
  else
    echo "Debian/Ubuntu: sudo apt install git wget flex bison gperf python3 python3-pip python3-venv cmake ninja-build ccache libffi-dev libssl-dev dfu-util libusb-1.0-0"
    echo "Fedora:        sudo dnf install git wget flex bison gperf python3 cmake ninja-build ccache dfu-util libusbx"
  fi
  exit 1
fi

if [ -d "$IDF_DIR/.git" ]; then
  have=$(git -C "$IDF_DIR" describe --tags 2>/dev/null || echo unknown)
  [ "$have" = "$IDF_VERSION" ] || { echo "$IDF_DIR holds ESP-IDF $have, not $IDF_VERSION. Set IDF_DIR to another folder."; exit 1; }
  echo "ESP-IDF $IDF_VERSION already cloned at $IDF_DIR"
else
  mkdir -p "$(dirname "$IDF_DIR")"
  echo "Downloading ESP-IDF $IDF_VERSION into $IDF_DIR (about 1 GB)..."
  git clone -b "$IDF_VERSION" --depth 1 --recursive --shallow-submodules https://github.com/espressif/esp-idf.git "$IDF_DIR"
fi

echo "Installing the ESP32-S3 toolchain (about 1-2 GB, a few minutes)..."
"$IDF_DIR/install.sh" esp32s3

if [ "$(uname)" = Linux ] && ! id -nG | grep -qw dialout; then
  echo
  echo "To use the robot's USB port without sudo, run once and log out/in:  sudo usermod -aG dialout $USER"
fi
cat <<EOF

Done. In every new terminal, before building:
    . "$IDF_DIR/export.sh"
Check:   idf.py --version     (should print ESP-IDF $IDF_VERSION)
Tip:     echo 'alias get_idf=". $IDF_DIR/export.sh"' >> ~/.bashrc   then just run get_idf
EOF
