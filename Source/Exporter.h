// SPDX-License-Identifier: GPL-3.0-only
#pragma once
#include "BuilderAssets.h"
#include "Model.h"
namespace pw {
// Four-character plugin code (AU subtype / VST3 class id) derived from the
// instrument UUID. Shared by the exporter and the builder's Export tab.
inline String pluginCode(const Document &d) {
  auto hash = static_cast<uint32>(d.uuid.hashCode());
  static constexpr char letters[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  String code = String::charToString(static_cast<juce_wchar>('A' + hash % 26));
  for (int i = 0; i < 3; ++i) {
    hash /= 26;
    code += String::charToString(static_cast<juce_wchar>(letters[hash % 62]));
  }
  return code;
}
// Plugin version that increases every minute, based on UTC so daylight saving
// and time zone changes never make it go backwards. JUCE packs minor and patch
// into 8 bits each for the AU version, so the minute count is split base 256.
inline String pluginVersion(Time when = Time::getCurrentTime()) {
  const int64 epoch = 1767225600000LL; // 2026-01-01 00:00 UTC
  const int64 minutes = std::max<int64>(0, (when.toMilliseconds() - epoch) / 60000);
  return String(1 + minutes / 65536) + "." + String((minutes / 256) % 256) +
         "." + String(minutes % 256);
}
// Per-user plugin folders that hosts scan by default, so installing never
// needs administrator rights:
//   macOS:   ~/Library/Audio/Plug-Ins/{VST3,Components,CLAP}
//   Windows: %LOCALAPPDATA%\Programs\Common\{VST3,CLAP}
//   Linux:   ~/.vst3, ~/.clap
// The folder names match the install rules in EngineTemplate/CMakeLists.txt.
inline File pluginInstallRoot() {
#if JUCE_MAC
  return File::getSpecialLocation(File::userHomeDirectory)
      .getChildFile("Library/Audio/Plug-Ins");
#elif JUCE_WINDOWS
  return File::getSpecialLocation(File::windowsLocalAppData)
      .getChildFile("Programs")
      .getChildFile("Common");
#else
  return File::getSpecialLocation(File::userHomeDirectory);
#endif
}
// format is "vst3", "au" or "clap".
inline String pluginFolderName(const String &format) {
#if JUCE_MAC
  return format == "au" ? "Components" : format.toUpperCase();
#elif JUCE_WINDOWS
  return format.toUpperCase();
#else
  return "." + format;
#endif
}
inline String pluginExtension(const String &format) {
  return format == "au" ? ".component" : "." + format;
}
// CMake used to build exported instruments: the one Plugweaver was built
// with, a standard install location, or the first one found on PATH.
inline File findCMake() {
  File built(PW_CMAKE_PATH);
  if (built.existsAsFile())
    return built;
#if JUCE_WINDOWS
  const String exe = "cmake.exe";
  StringArray candidates{"C:\\Program Files\\CMake\\bin\\cmake.exe"};
  const String separator = ";";
#else
  const String exe = "cmake";
  StringArray candidates{"/Applications/CMake.app/Contents/bin/cmake",
                         "/opt/homebrew/bin/cmake", "/usr/local/bin/cmake",
                         "/usr/bin/cmake"};
  const String separator = ":";
#endif
  for (auto &dir : StringArray::fromTokens(
           SystemStats::getEnvironmentVariable("PATH", {}), separator, "\""))
    if (dir.isNotEmpty())
      candidates.add(File(dir).getChildFile(exe).getFullPathName());
  for (auto &path : candidates)
    if (File(path).existsAsFile())
      return File(path);
  return {};
}
// This only extracts the trusted code archive compiled into Plugweaver, never a
// user template.
inline File exportInstrument(const Document &d, const File &parent, bool vst3,
                             bool au, bool clap) {
  require(!d.zones.empty(), "Import an SFZ instrument before building");
  // The name becomes the plugin bundle name and is used by the install step,
  // so characters with meaning to CMake ($ { } [ ] \ ") are not allowed.
  require(d.name.trim().isNotEmpty() && d.name == d.name.trim() &&
              d.name.length() <= 80 && d.maker.length() <= 80 &&
              d.name == File::createLegalFileName(d.name) &&
              !d.name.containsAnyOf(";\r\n${}[]\\\"") &&
              !d.maker.containsAnyOf(";\r\n"),
          "Use an instrument name under 80 characters without path separators, "
          "line breaks or the characters $ { } [ ] \\ \"");
  require(vst3 || au || clap, "Select at least one plugin format");
  auto safeName = d.name.retainCharacters(
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_");
  if (safeName.isEmpty())
    safeName = "Instrument";
  File target = parent.getNonexistentChildFile(safeName + "-Source", "", false);
  require(target.createDirectory().wasOk(), "Cannot create export directory");
  try {
    MemoryInputStream in(
        BuilderAssets::engine_template_zip,
        static_cast<std::size_t>(BuilderAssets::engine_template_zipSize),
        false);
    ZipFile zip(in);
    require(zip.uncompressTo(target, false).wasOk(),
            "Could not unpack source template");
    saveDocument(d, target.getChildFile("instrument.zip"));
    // Bracket quoting prevents instrument names being interpreted as CMake
    // code.
    auto quoted = [](String s) {
      require(!s.contains("]====]"), "Unsupported text in instrument name");
      return "[====[" + s + "]====]";
    };
    String code = pluginCode(d);
    String id = "org.artilody.instrument." +
                d.uuid.retainCharacters("0123456789abcdef");
    String version = pluginVersion();
    String config = "set(PW_NAME " + quoted(d.name) + ")\nset(PW_MAKER " +
                    quoted(d.maker) + ")\nset(PW_ID " + quoted(id) +
                    ")\nset(PW_CODE " + code + ")\nset(PW_VST3 " +
                    String(vst3 ? "ON" : "OFF") + ")\nset(PW_AU " +
                    String(au ? "ON" : "OFF") + ")\nset(PW_CLAP " +
                    String(clap ? "ON" : "OFF") + ")\nset(PW_VERSION " +
                    version + ")\n";
    require(
        target.getChildFile("InstrumentConfig.cmake").replaceWithText(config),
        "Cannot write instrument configuration");
    target.getChildFile("Build-Mac.command").setExecutePermission(true);
    target.getChildFile("Build-Linux.sh").setExecutePermission(true);
    return target;
  } catch (...) {
    target.deleteRecursively();
    throw;
  }
}
} // namespace pw
