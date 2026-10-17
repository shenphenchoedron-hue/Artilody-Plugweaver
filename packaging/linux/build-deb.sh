#!/bin/bash
# Builds the Debian/Ubuntu package for Artilody Plugweaver.
# Usage: build-deb.sh <path to binary> <version> <output .deb>
set -euo pipefail
binary="$1"; version="$2"; out="$3"
here="$(cd "$(dirname "$0")" && pwd)"
root="$(mktemp -d)"
trap 'rm -rf "$root"' EXIT
install -Dm755 "$binary" "$root/opt/artilody-plugweaver/artilody-plugweaver"
install -Dm644 "$here/../../LICENSE" "$root/opt/artilody-plugweaver/LICENSE"
mkdir -p "$root/usr/bin"
ln -s /opt/artilody-plugweaver/artilody-plugweaver "$root/usr/bin/artilody-plugweaver"
install -Dm644 "$here/../../Assets/icon.png" "$root/usr/share/pixmaps/artilody-plugweaver.png"
install -Dm644 /dev/stdin "$root/usr/share/applications/artilody-plugweaver.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=Artilody Plugweaver
Comment=Create sampler instrument plugins (VST3, CLAP) from SFZ
Exec=artilody-plugweaver
Icon=artilody-plugweaver
Categories=AudioVideo;Audio;Music;
Terminal=false
EOF
size="$(du -sk "$root" | cut -f1)"
install -Dm644 /dev/stdin "$root/DEBIAN/control" <<EOF
Package: artilody-plugweaver
Version: $version
Section: sound
Priority: optional
Architecture: amd64
Maintainer: Artilody <238510944+shenphenchoedron-hue@users.noreply.github.com>
Installed-Size: $size
Depends: libc6 (>= 2.35), libasound2 | libasound2t64, libfreetype6, libfontconfig1, libx11-6, libxext6, libxrandr2, libxinerama1, libxcursor1
Homepage: https://github.com/shenphenchoedron-hue/Artilody-Plugweaver
Description: Instrument builder for VST3 and CLAP sampler plugins
 Artilody Plugweaver imports SFZ instruments, lets you shape the sound and
 design the plugin, and builds finished VST3 and CLAP plugins directly,
 without a compiler or other tools.
EOF
dpkg-deb --root-owner-group --build "$root" "$out"
echo "Package: $out"
