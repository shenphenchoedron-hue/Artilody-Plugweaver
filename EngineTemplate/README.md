# Generated instrument — Artilody Plugweaver

This complete C++20/JUCE 7.0.12 project contains its samples and design in instrument.zip.
Build on Mac with `bash Build-Mac.command`, on Linux with `bash Build-Linux.sh`, on Windows by
double-clicking `Build-Windows.bat`, or use CMake and the included GitHub Actions workflow
(Mac, Windows and Linux).
The chosen VST3/AU/CLAP formats and a standalone instrument are built. AU is Mac-only.
Outputs are in build/Instrument_artefacts/Release (or build-mac/... for the command script).
No Plugweaver installation is required by the resulting plugin.

The generated C++ is GPL-3.0-only. When distributing binaries, provide the corresponding source,
including instrument.zip and the build configuration, under the applicable GPLv3 obligations.
The JUCE modules and bundled VST3 SDK use their GPLv3 option; CLAP and its JUCE extension are MIT.
Use only samples and images whose terms allow your intended distribution. No proprietary SDK is needed.
This does not require a paid JUCE licence, subscription, Apple Developer membership or cloud build service.
The software is not Apple-notarized; ad-hoc signing on your own Mac is supported.

On Mac, Plugweaver's Build plugin button and Build-Mac.command build universal Intel/Apple Silicon
plugins, sign them locally, and install the selected formats into your user's standard plugin folders:

- VST3: `~/Library/Audio/Plug-Ins/VST3/`
- AU: `~/Library/Audio/Plug-Ins/Components/`
- CLAP: `~/Library/Audio/Plug-Ins/CLAP/`

On Windows and Linux the build scripts and the Build plugin button install into the per-user folders:

- Windows: `%LOCALAPPDATA%\Programs\Common\VST3\` and `...\CLAP\`
- Linux: `~/.vst3/` and `~/.clap/`

Windows needs Visual Studio 2022 (Build Tools, "Desktop development with C++"), CMake 3.24+
and Git (used to download the CLAP extension).
Linux needs GCC/Clang, CMake 3.24+ and the ALSA, JACK, X11, FreeType and OpenGL development
packages listed in Build-Linux.sh.

Restart your host and rescan plugins after installation. These are instruments, not audio effects.
Logic uses AU; other hosts need support for the selected format. Pro Tools AAX is not supported.
When building directly with CMake or downloading CI artifacts, sign and copy the complete plugin
bundles to these folders yourself. A standalone .app cannot be loaded as a plugin.
Keep source projects for each instrument separate. UUID and generated plugin code remain stable
when exporting the same saved Plugweaver project. Do not reuse a copied project UUID for unrelated instruments.
