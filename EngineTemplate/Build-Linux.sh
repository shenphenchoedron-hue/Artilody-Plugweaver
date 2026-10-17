#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
trap 'printf "\nBuild fejlede. Kopiér fejlbeskeden ovenfor, så vi kan rette den.\n"' ERR
if [[ "$(uname -s)" != "Linux" ]]; then
  echo "Denne fil bygger Linux-versionen og skal køres på Linux."
  exit 1
fi
if ! command -v cmake >/dev/null 2>&1 || ! command -v g++ >/dev/null 2>&1; then
  echo "CMake og en C++-compiler mangler. På Ubuntu/Debian:"
  echo "  sudo apt install build-essential cmake git pkg-config libasound2-dev libjack-jackd2-dev \\"
  echo "    libfreetype-dev libfontconfig1-dev libx11-dev libxcomposite-dev libxcursor-dev \\"
  echo "    libxext-dev libxinerama-dev libxrandr-dev libxrender-dev libgl1-mesa-dev"
  exit 1
fi
build_dir="$PWD/build-linux"
echo "Bygger instrumentet — VST3 og/eller CLAP."
cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Release
cmake --build "$build_dir" --config Release --parallel "$(nproc)"
cmake --install "$build_dir" --config Release --component Plugins --prefix "$HOME"
echo "Plugins installeret i ~/.vst3 og ~/.clap."
echo "Luk musikprogrammet, åbn det igen, og scan plugins igen."
echo "Færdig: $build_dir/Instrument_artefacts/Release"
