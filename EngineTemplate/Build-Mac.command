#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")"
trap 'printf "\nBuild fejlede. Kopiér fejlbeskeden ovenfor, så vi kan rette den.\n"' ERR
if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "Denne fil bygger Mac-versionen og skal køres på din Mac."
  exit 1
fi
if ! xcode-select -p >/dev/null 2>&1 || ! xcrun --find clang++ >/dev/null 2>&1; then
  echo "Apples Command Line Tools mangler. Installationsdialogen åbnes nu."
  xcode-select --install || true
  echo "Kør buildfilen igen efter installationen."
  exit 1
fi
project_dir="$PWD"
cmake_bin=""
if command -v cmake >/dev/null 2>&1; then
  cmake_bin="$(command -v cmake)"
elif [[ -x /Applications/CMake.app/Contents/bin/cmake ]]; then
  cmake_bin=/Applications/CMake.app/Contents/bin/cmake
fi
# Use a recent existing CMake, otherwise keep a verified private copy in this project.
if [[ -n "$cmake_bin" ]]; then
  version="$("$cmake_bin" --version | head -1 | awk '{print $3}')"
  major="${version%%.*}"; remaining="${version#*.}"; minor="${remaining%%.*}"
  if [[ "$major" -lt 3 || ( "$major" -eq 3 && "$minor" -lt 24 ) ]]; then cmake_bin=""; fi
fi
if [[ -z "$cmake_bin" ]]; then
  tool_dir="$project_dir/.tools"
  cmake_bin="$tool_dir/cmake-3.31.6-macos-universal/CMake.app/Contents/bin/cmake"
  if [[ ! -x "$cmake_bin" ]]; then
    mkdir -p "$tool_dir"
    archive="$tool_dir/cmake-3.31.6-macos-universal.tar.gz"
    echo "Henter CMake 3.31.6 til projektets egen værktøjsmappe…"
    curl --fail --location --retry 2 \
      https://github.com/Kitware/CMake/releases/download/v3.31.6/cmake-3.31.6-macos-universal.tar.gz \
      --output "$archive"
    expected=330b9514f5112e5ed4fb08b8b05803b776fd9b539a6ae12927d14dcc0ee2ba8d
    actual="$(shasum -a 256 "$archive" | awk '{print $1}')"
    if [[ "$actual" != "$expected" ]]; then
      echo "CMake-downloadets checksum matcher ikke. Build stoppet."
      exit 1
    fi
    tar -xzf "$archive" -C "$tool_dir"
  fi
fi
build_dir="$project_dir/build-mac"
echo "Bygger instrumentet — VST3, AU og/eller CLAP."
"$cmake_bin" -S "$project_dir" -B "$build_dir" -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64'
"$cmake_bin" --build "$build_dir" --config Release --parallel "$(sysctl -n hw.ncpu)"
while IFS= read -r -d '' bundle; do
  xattr -cr "$bundle" || true
  codesign --force --deep --sign - "$bundle"
  codesign --verify --deep --strict "$bundle"
done < <(find "$build_dir/Instrument_artefacts/Release" -type d \( -name '*.vst3' -o -name '*.component' -o -name '*.clap' -o -name '*.app' \) -prune -print0)
"$cmake_bin" --install "$build_dir" --config Release --component Plugins --prefix "$HOME/Library/Audio/Plug-Ins"
# Let macOS re-read its Audio Unit list so Logic, GarageBand and others see the new AU.
killall -9 AudioComponentRegistrar >/dev/null 2>&1 || true
echo "Plugins installeret i $HOME/Library/Audio/Plug-Ins."
echo "Luk musikprogrammet helt, åbn det igen, og scan plugins igen (Logic: Plug-in Manager > Reset & Rescan;"
echo "REAPER: Preferences > Plug-ins > Re-scan; FL Studio: Options > Manage plugins > Find installed plugins)."
echo "Færdig: $build_dir/Instrument_artefacts/Release"
open "$build_dir/Instrument_artefacts/Release"
