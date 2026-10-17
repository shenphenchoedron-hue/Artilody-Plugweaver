#!/bin/bash
# Builds the macOS installer: Artilody Plugweaver.app into /Applications.
# Usage: build-pkg.sh <path to .app> <version> <output .pkg>
set -euo pipefail
app="$1"; version="$2"; out="$3"
work="$(mktemp -d)"
trap 'rm -rf "$work"' EXIT
mkdir -p "$work/root"
ditto "$app" "$work/root/$(basename "$app")"
codesign --force --deep --sign - "$work/root/$(basename "$app")"
codesign --verify --deep --strict "$work/root/$(basename "$app")"
pkgbuild --analyze --root "$work/root" "$work/components.plist"
# Always install into /Applications, even if another copy exists elsewhere.
/usr/libexec/PlistBuddy -c "Set :0:BundleIsRelocatable false" "$work/components.plist"
pkgbuild --root "$work/root" --component-plist "$work/components.plist" \
  --install-location /Applications --identifier org.artilody.plugweaver \
  --version "$version" "$work/app.pkg"
cat > "$work/distribution.xml" <<EOF
<?xml version="1.0" encoding="utf-8"?>
<installer-gui-script minSpecVersion="2">
  <title>Artilody Plugweaver $version</title>
  <options customize="never" require-scripts="false" hostArchitectures="x86_64,arm64"/>
  <volume-check><allowed-os-versions><os-version min="11.0"/></allowed-os-versions></volume-check>
  <choices-outline><line choice="app"/></choices-outline>
  <choice id="app" title="Artilody Plugweaver"><pkg-ref id="org.artilody.plugweaver"/></choice>
  <pkg-ref id="org.artilody.plugweaver" version="$version">app.pkg</pkg-ref>
</installer-gui-script>
EOF
productbuild --distribution "$work/distribution.xml" --package-path "$work" "$out"
echo "Installer: $out"
