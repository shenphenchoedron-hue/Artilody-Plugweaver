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
echo "Bygger Artilody Plugweaver — C++ / JUCE 7.0.12 / GPLv3."
cmake -S . -B "$build_dir" -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build "$build_dir" --config Release --parallel "$(nproc)"
ctest --test-dir "$build_dir" -C Release --output-on-failure
echo "Færdig: $build_dir/Plugweaver_artefacts/Release/Artilody Plugweaver"
